#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]

include!(concat!(env!("OUT_DIR"), "/path_bindings.rs"));

fn main() {
    ;
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn round_trip_serialize() {
        let mut x = Vec::<f32>::with_capacity(100);
        let mut y = Vec::<f32>::with_capacity(100);

        for i in 0..100 {
            x.push(i as f32 * 1_f32);
            y.push(i as f32 * 2_f32);
        }

        unsafe {
            let data = create_path(3);
            assert!(!data.is_null());

            assert!(set_path_x(data, x.as_mut_ptr(), 100));
            assert!(set_path_y(data, y.as_mut_ptr(), 100));

            let serialized = serialize_path(data as *mut std::ffi::c_void);
            assert!(!serialized.is_null());
            destroy_path(data);

            let deserialized = deserialize_path(serialized);
        }
    }
}