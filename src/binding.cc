#include <nan.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>

void GetGroups(const Nan::FunctionCallbackInfo<v8::Value>& info) {
  gid_t *groupList;
  int ngroups = 0, i = 0;

  ngroups = sysconf(_SC_NGROUPS_MAX);

  groupList = (gid_t *)calloc(ngroups, sizeof(gid_t));
  assert(groupList != NULL);

  ngroups = getgroups(ngroups, groupList);
  assert(ngroups != -1);

  v8::Local<v8::Array> groupsArray = Nan::New<v8::Array>();

  for (i = 0; i < ngroups; i++) {
    groupsArray->Set(Nan::GetCurrentContext(),
      i, Nan::New<v8::Integer>(groupList[i])
    ).Check();
  }

  info.GetReturnValue().Set(groupsArray);
  return;
}

void InitGroups(const Nan::FunctionCallbackInfo<v8::Value>& info) {

  if (info.Length() < 1) {
    Nan::ThrowTypeError("initgroups requires 1 argument");
    return;
  }

  int err = 0, bufsize = 0;
  gid_t gid = 0;
  Nan::Utf8String pwnam(info[0]);
  if (!*pwnam) {
    Nan::ThrowError("initgroups user must be provided");
    return;
  }
  struct passwd pwd, *pwdp = NULL;
  bufsize = sysconf(_SC_GETPW_R_SIZE_MAX);
  char buffer[bufsize];

  errno = 0;
  if ((err = getpwnam_r(*pwnam, &pwd, buffer, bufsize, &pwdp)) ||
      pwdp == NULL) {
    if (errno == 0) {
      Nan::ThrowError("initgroups user does not exist");
      return;
    }
    else {
      Nan::ThrowError(Nan::ErrnoException(errno, "getpwnam_r"));
      return;
    }
  }

  gid = pwd.pw_gid;
  if (info.Length() > 1 && info[1]->IsTrue()) {
    if ((err = setgid(gid)) == -1) {
      Nan::ThrowError(Nan::ErrnoException(errno, "setgid"));
      return;
    }
  }

  if ((err = initgroups(*pwnam, gid)) == -1) {
    Nan::ThrowError(Nan::ErrnoException(errno, "initgroups"));
    return;
  }

  info.GetReturnValue().Set(Nan::Undefined());
  return;
}

NAN_MODULE_INIT(Initialize) {
  Nan::Export(target, "initgroups", InitGroups);
  Nan::Export(target, "getgroups", GetGroups);
}

NODE_MODULE(unixgroups, Initialize);
