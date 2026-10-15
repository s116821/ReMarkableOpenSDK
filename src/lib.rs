//! Persistent identity primitives. No native operations or observation authority.
#![forbid(unsafe_code)]

/// Persistent identity; operational device/instance scope is carried separately.
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub struct PageKey {
    pub document: Uuid,
    pub page: Uuid,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub struct Uuid([u8; 16]);

impl Uuid {
    pub fn as_bytes(&self) -> &[u8; 16] {
        &self.0
    }
    /// Accept canonical lowercase, nonnil UUID text; indices/titles are not IDs.
    pub fn parse(text: &str) -> Option<Self> {
        if text.len() != 36 {
            return None;
        }
        let mut bytes = [0; 16];
        let mut digits = 0;
        for (index, c) in text.bytes().enumerate() {
            if [8, 13, 18, 23].contains(&index) {
                if c != b'-' {
                    return None;
                }
            } else {
                let value = match c {
                    b'0'..=b'9' => c - b'0',
                    b'a'..=b'f' => c - b'a' + 10,
                    _ => return None,
                };
                bytes[digits / 2] = (bytes[digits / 2] << 4) | value;
                digits += 1;
            }
        }
        (bytes != [0; 16]).then_some(Self(bytes))
    }
}

impl std::fmt::Display for Uuid {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        for (index, byte) in self.0.iter().enumerate() {
            if [4, 6, 8, 10].contains(&index) {
                f.write_str("-")?;
            }
            write!(f, "{byte:02x}")?;
        }
        Ok(())
    }
}

#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub struct OperationId(pub Uuid);

/// Ordinary gesture plans only.
pub mod navigation;
