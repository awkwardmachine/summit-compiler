; ModuleID = 'summit'
source_filename = "summit"

%module_t = type opaque
%module_t.0 = type opaque

@lib = constant %module_t zeroinitializer
@out = constant %module_t.0 zeroinitializer
@0 = private unnamed_addr constant [18 x i8] c"Enter your name: \00", align 1
@1 = private unnamed_addr constant [15 x i8] c"Your name is: \00", align 1

define i1 @__user_main() {
entry:
  %x = alloca ptr, align 8
  call void @io_print_str(ptr @0)
  %0 = call ptr @io_readln()
  store ptr %0, ptr %x, align 8
  %x1 = load ptr, ptr %x, align 8
  %1 = call i64 @strlen(ptr @1)
  %2 = call i64 @strlen(ptr %x1)
  %3 = add i64 %1, %2
  %4 = add i64 %3, 1
  %5 = call ptr @malloc(i64 %4)
  %6 = call ptr @strcpy(ptr %5, ptr @1)
  %7 = call ptr @strcat(ptr %5, ptr %x1)
  call void @io_println_str(ptr %5)
  ret i1 false
}

declare void @io_print_str(ptr)

declare ptr @io_readln()

declare void @io_println_str(ptr)

declare i64 @strlen(ptr)

declare ptr @malloc(i64)

declare ptr @strcpy(ptr, ptr)

declare ptr @strcat(ptr, ptr)

define i32 @main() {
entry:
  %0 = call i1 @__user_main()
  %1 = zext i1 %0 to i32
  ret i32 %1
}
