use globwalk::{self, GlobWalker, WalkError};

pub struct DirWalker {
    pub walker: Option<GlobWalker>,
}

impl DirWalker {
    pub fn from_patterns<P, S>(base: P, patterns: &[S]) -> Self
    where
        P: AsRef<std::path::Path>,
        S: AsRef<str>,
    {
        let walker = globwalk::GlobWalkerBuilder::from_patterns(base, patterns)
            .build()
            .ok();

        Self { walker }
    }
}

impl Iterator for DirWalker {
    type Item = Result<std::path::PathBuf, WalkError>;

    fn next(&mut self) -> Option<Self::Item> {
        self.walker
            .as_mut()?
            .next()
            .map(|dir_entry| dir_entry.map(|x| x.into_path()))
    }
}
