#include "node.h"
#include "node_buffer.h"
#include "v8.h"
#include "nan.h"

#include <sys/mman.h>
#include <unistd.h>

namespace node_mmap {

using v8::Handle;
using v8::Local;
using v8::Number;
using v8::Object;
using v8::Value;

static void FreeCallback(char* data, void* hint) {
  int err;

  err = munmap(data, reinterpret_cast<intptr_t>(hint));
  assert(err == 0);
}


static void DontFree(char* data, void* hint) {
}


NAN_METHOD(Alloc) {
  NanEscapableScope();

  int len = args[0]->Uint32Value();
  int prot = args[1]->Uint32Value();
  int flags = args[2]->Uint32Value();
  int fd = args[3]->Int32Value();
  int off = args[4]->Uint32Value();

  void* res = mmap(NULL, len, prot, flags, fd, off);
  if (res == MAP_FAILED)
    return NanThrowError("mmap() call failed");

  return NanEscapeScope(NanNewBufferHandle(
        reinterpret_cast<char*>(res),
        len,
        FreeCallback,
        reinterpret_cast<void*>(static_cast<intptr_t>(len))));
}


NAN_METHOD(AlignedAlloc) {
  NanEscapableScope();

  int len = args[0]->Uint32Value();
  int prot = args[1]->Uint32Value();
  int flags = args[2]->Uint32Value();
  int fd = args[3]->Int32Value();
  int off = args[4]->Uint32Value();
  int align = args[5]->Uint32Value();

  void* res = mmap(NULL, len + align, prot, flags, fd, off);
  if (res == MAP_FAILED)
    return NanThrowError("mmap() call failed");

  Local<Object> buf = NanNewBufferHandle(
        reinterpret_cast<char*>(res),
        len,
        FreeCallback,
        reinterpret_cast<void*>(static_cast<intptr_t>(len + align)));

  intptr_t ptr = reinterpret_cast<intptr_t>(res);
  if (ptr % align == 0) {
    return NanEscapeScope(buf);
  }

  // Slice the buffer if it was unaligned
  int slice_off = align - (ptr % align);
  Local<Object> slice = NanNewBufferHandle(
      reinterpret_cast<char*>(res) + slice_off,
      len,
      DontFree,
      NULL);

  slice->SetHiddenValue(NanNew("alignParent"), buf);
  return NanEscapeScope(slice);
}


static void Init(Handle<Object> target) {
  NODE_SET_METHOD(target, "alloc", Alloc);
  NODE_SET_METHOD(target, "alignedAlloc", AlignedAlloc);

  target->Set(NanNew("PROT_NONE"), NanNew<Number, uint32_t>(PROT_NONE));
  target->Set(NanNew("PROT_READ"), NanNew<Number, uint32_t>(PROT_READ));
  target->Set(NanNew("PROT_WRITE"), NanNew<Number, uint32_t>(PROT_WRITE));
  target->Set(NanNew("PROT_EXEC"), NanNew<Number, uint32_t>(PROT_EXEC));

  target->Set(NanNew("MAP_ANON"), NanNew<Number, uint32_t>(MAP_ANON));
  target->Set(NanNew("MAP_PRIVATE"), NanNew<Number, uint32_t>(MAP_PRIVATE));
  target->Set(NanNew("MAP_SHARED"), NanNew<Number, uint32_t>(MAP_SHARED));
  target->Set(NanNew("MAP_FIXED"), NanNew<Number, uint32_t>(MAP_FIXED));

  target->Set(NanNew("PAGE_SIZE"), NanNew<Number, uint32_t>(getpagesize()));
}


NODE_MODULE(mmap, Init);

}  // namespace node_mmap
