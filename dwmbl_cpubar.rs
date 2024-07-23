use std::{
    env::args,
    ffi::c_long,
    fmt::{self, Display, Formatter},
    fs::File,
    io::{BufRead, BufReader, BufWriter, Read, Write},
    ops::Sub,
    process::exit,
    time::Duration,
};

extern "C" {
    fn total_threads() -> c_long;
    fn ticks_per_second() -> c_long;
}

/// Uptime, idle time for each core and overall average idle time.
struct CpuInfo {
    uptime: Duration,
    idle: Duration,
    core_idle: Vec<Duration>,
}

/// The number of bars.
const BAR_GRANUARITY: usize = 9;
/// Bars that can be used to visually represent core usage.
const BARS: &str = "█▇▆▅▄▃▂▁ ";

impl CpuInfo {
    /// The cache that is used to save current CPU info.
    const CACHE_FILE: &'static str = "/tmp/cpubarcache";
}

impl Display for CpuInfo {
    fn fmt(&self, f: &mut Formatter<'_>) -> fmt::Result {
        let uptime_secs = self.uptime.as_secs_f64();
        let idle_secs = self.idle.as_secs_f64().clamp(0.0, uptime_secs);
        let fraction = if uptime_secs == 0.0 {
            0.0
        } else {
            1.0 - idle_secs / uptime_secs
        };

        write!(f, "CPU: {:.2}%", fraction * 100.0)?;

        if !self.core_idle.is_empty() {
            f.write_str(" ")?;

            for core_idle_time in &self.core_idle {
                let fraction = if uptime_secs == 0.0 {
                    1.0
                } else {
                    core_idle_time.as_secs_f64().clamp(0.0, uptime_secs) / uptime_secs
                };
                // 0..BAR_GRANUARITY
                let index = ((BAR_GRANUARITY - 1) as f64 * fraction).round() as usize;
                let bar = BARS.chars().nth(index).unwrap();
                write!(f, "{bar}")?;
            }
        }

        Ok(())
    }
}

impl Sub for CpuInfo {
    type Output = Self;

    fn sub(mut self, rhs: Self) -> Self::Output {
        self.uptime -= rhs.uptime;
        self.idle -= rhs.idle;

        for (core_idle_time, old_core_idle_time) in
            self.core_idle.iter_mut().zip(rhs.core_idle.iter())
        {
            *core_idle_time -= *old_core_idle_time;
        }

        self
    }
}

impl CpuInfo {
    /// Creates new [`CpuInfo`] from the information in `/proc/uptime` and
    /// `/proc/stat`.
    ///
    /// Technically it could be done with only `/proc/stat` but that would
    /// require converting a unix timestamp which I don't want to do.
    fn new(show_visual_cores: bool) -> Self {
        // SAFETY: this calls sysconf(), which is harmless
        let total_threads = unsafe { total_threads() };
        // SAFETY: this calls sysconf(), which is harmless
        let ticks_per_second = unsafe { ticks_per_second() };

        let uptime_file = File::open("/proc/uptime").expect("could not open /proc/uptime");
        let uptime_file = BufReader::new(uptime_file);

        let stats_file = File::open("/proc/stat").expect("could not open /proc/stat");
        let stats_file = BufReader::new(stats_file);

        let uptime_line = uptime_file.lines().next().unwrap().unwrap();
        let mut uptime_tokens = uptime_line.split_whitespace();
        let uptime_secs = uptime_tokens.next().unwrap().parse().unwrap();
        let uptime = Duration::from_secs_f64(uptime_secs);
        let idle_secs = uptime_tokens.next().unwrap().parse().unwrap();
        let idle = Duration::from_secs_f64(idle_secs) / total_threads as u32;

        let mut core_idle = Vec::with_capacity(total_threads as usize);

        if show_visual_cores {
            for line in stats_file.lines().map(Result::unwrap) {
                let mut tokens = line.split_whitespace();

                match tokens.next() {
                    // skip the first line
                    Some("cpu") => continue,
                    // stop once we're no longer reading info for each core
                    Some("intr") => break,
                    _ => (),
                }

                // get the idle time of each core (the 5th column)
                let core_idle_ticks = tokens.nth(3).and_then(|c| c.parse().ok()).unwrap();
                let core_idle_time = Duration::from_secs(core_idle_ticks) / ticks_per_second as u32;
                core_idle.push(core_idle_time);
            }
        }

        Self {
            uptime,
            idle,
            core_idle,
        }
    }

    /// Creates new [`CpuInfo`] from the information in [`Self::CACHE_FILE`].
    ///
    /// [`Self::CACHE_FILE`] stores the information as `f64`'s: uptime, average
    /// idle time and idle time for each core in order. As such, it has size
    /// `size_of::<f64>() * (total_cores + 2)`.
    fn from_cache() -> Option<Self> {
        let Ok(cache_file) = File::open(Self::CACHE_FILE) else {
            return None;
        };
        let mut cache_file = BufReader::new(cache_file);

        let mut bytes = Vec::new();
        cache_file.read_to_end(&mut bytes).unwrap();
        if bytes.len() % 8 != 0 {
            return None;
        }
        let cores = (bytes.len() / 8).checked_sub(2).unwrap();
        let mut core_idle: Vec<Duration> = Vec::with_capacity(cores);

        let uptime = f64::from_ne_bytes(bytes[0..8].try_into().unwrap());
        let idle = f64::from_ne_bytes(bytes[8..16].try_into().unwrap());
        for core in (0..cores).map(|c| (c + 2) * 8) {
            let core_idle_f64 = f64::from_ne_bytes(bytes[core..(core + 8)].try_into().unwrap());
            core_idle.push(Duration::from_secs_f64(core_idle_f64));
        }

        Some(Self {
            uptime: Duration::from_secs_f64(uptime),
            idle: Duration::from_secs_f64(idle),
            core_idle,
        })
    }

    /// Stores the current information into [`Self::CACHE_FILE`].
    ///
    /// See its documentation for more detail.
    fn cache(&self) {
        let cache_file =
            File::create(Self::CACHE_FILE).expect("could not open CpuInfo::CACHE_FILE for writing");
        let mut cache_file = BufWriter::new(cache_file);

        let uptime_secs_u8 = self.uptime.as_secs_f64().to_ne_bytes();
        let idle_secs_u8 = self.idle.as_secs_f64().to_ne_bytes();
        cache_file
            .write_all(&uptime_secs_u8)
            .expect("could not write to CpuInfo::CACHE_FILE");
        cache_file
            .write_all(&idle_secs_u8)
            .expect("could not write to CpuInfo::CACHE_FILE");

        for core_idle_time in &self.core_idle {
            let idle_time_u8 = core_idle_time.as_secs_f64().to_ne_bytes();
            cache_file
                .write_all(&idle_time_u8)
                .expect("could not write to CpuInfo::CACHE_FILE");
        }
    }
}

fn main() {
    let mut args = args();
    let name = args.next().expect("no program name, somehow");
    let mut show_visual_cores = false;

    for arg in args {
        match arg.as_str() {
            "-h" => usage(&name),
            "-v" => show_visual_cores = true,
            _ => (),
        }
    }

    let cpu_info = CpuInfo::new(show_visual_cores);
    let cached_info = CpuInfo::from_cache();
    cpu_info.cache();
    let Some(cached_info) = cached_info else {
        exit(0);
    };
    let diff = cpu_info - cached_info;
    println!("{diff}");
}

/// Prints usage and exits.
fn usage(name: &str) {
    println!("Usage: {name} [options]");
    println!("Options:");
    println!("  -v           Show visual cores");
    println!("  -h           Print this message");
    exit(0);
}
