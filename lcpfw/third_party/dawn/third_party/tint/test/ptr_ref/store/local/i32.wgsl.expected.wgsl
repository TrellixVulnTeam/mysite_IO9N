[[stage(compute)]]
fn main() {
  var i : i32 = 123;
  let p : ptr<function, i32> = &(i);
  *(p) = 123;
  *(p) = ((100 + 20) + 3);
}
