## Example 
``` c
extern {
    fn malloc(int i) -> *void {}
    fn printf(*char string,int val) {}
}

fn foo([]int arr, int i) {
    if arr.length > i  {
        arr[1] = 3;
    }
}
fn main(int argc, **char argv) -> int {
    "hello";
    arr_1: [10]int;
    arr_1[0] = 420;

    arr_1_ptr:*[]int= &arr_1;

    arr_2: [10]*[10]int;
    arr_2[0] = arr_1_ptr;

    abc: *[]*[]int = &arr_2;

    arr_2_deref: []*[]int= *abc;

    arr_2_elem: *[]int = arr_2_deref[0];

    arr_1_deref:= *arr_2_elem;
    printf("val:%d",arr_1_deref[0]);
    a:int;
    b:int;
    g:int;
    for i:int = 1; i < b; ++i {
            i + b;
        }
    while a+a > a {
        x: int;
        x > c;
        return &a;
    }
    if g > g  {
        c: int;
        if( b > c) {
        }
    }
    i + b;
    if ( b > b ) {
        return &a;
    }
    {
        a: int;
        b: int;
    }

}
```
