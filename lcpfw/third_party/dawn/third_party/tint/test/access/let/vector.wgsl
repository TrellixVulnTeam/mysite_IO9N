[[stage(compute)]]
fn main() {
  let v : vec3<f32> = vec3<f32>(1., 2., 3.);
  let scalar : f32 = v.y;
  let swizzle2 : vec2<f32> = v.xz;
  let swizzle3 : vec3<f32> = v.xzy;
}
