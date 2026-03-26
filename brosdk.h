#if !defined(__BROSDK_H__)
#define __BROSDK_H__
/* Minimal public C API header - keep footprint small but include C types
  needed by language bindings (cgo, ctypes, etc.). */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#if defined(SDK_API)
#undef SDK_API
#endif
#if defined(SDK_CALL)
#undef SDK_CALL
#endif

#if defined(_WIN32) || defined(_WIN64)
#define SDK_API __declspec(dllexport)
#define SDK_CALL __cdecl
#elif defined(__clang__) || defined(__GNUC__)
#define SDK_API __attribute__((visibility("default")))
#define SDK_CALL
#else
#define SDK_API
#define SDK_CALL
#endif

#ifdef __cplusplus
extern "C" {
#endif
typedef void *sdk_handle_t;
typedef void(SDK_CALL *sdk_result_cb_t)(int32_t code, void *user_data,
                                        const char *data, size_t len);
typedef void(SDK_CALL *sdk_cookies_storage_cb_t)(const char *data, size_t len,
                                                 char **new_data,
                                                 size_t *new_len,
                                                 void *user_data);
SDK_API int32_t SDK_CALL sdk_register_result_cb(sdk_result_cb_t cb,
                                                void *user_data);
SDK_API int32_t SDK_CALL
sdk_register_cookies_storage_cb(sdk_cookies_storage_cb_t cb, void *user_data);
SDK_API int32_t SDK_CALL sdk_init(sdk_handle_t *cpp_handle, const char *data,
                                  size_t len, char **out_data, size_t *out_len);
SDK_API int32_t SDK_CALL sdk_info(char **out_data, size_t *out_len);
SDK_API int32_t SDK_CALL sdk_browser_install(const char *data, size_t len);
SDK_API int32_t SDK_CALL sdk_browser_info(char **out_data, size_t *out_len);
SDK_API int32_t SDK_CALL sdk_browser_open(const char *data, size_t len);
SDK_API int32_t SDK_CALL sdk_browser_close(const char *data, size_t len);
SDK_API int32_t SDK_CALL sdk_env_create(const char *data, size_t len,
                                        char **out_data, size_t *out_len);
SDK_API int32_t SDK_CALL sdk_env_update(const char *data, size_t len,
                                        char **out_data, size_t *out_len);
SDK_API int32_t SDK_CALL sdk_env_page(const char *data, size_t len,
                                      char **out_data, size_t *out_len);
SDK_API int32_t SDK_CALL sdk_env_destroy(const char *data, size_t len,
                                         char **out_data, size_t *out_len);
SDK_API int32_t SDK_CALL sdk_token_update(const char *data, size_t len);
SDK_API int32_t SDK_CALL sdk_shutdown(void);
SDK_API void SDK_CALL sdk_free(void *ptr);
SDK_API void *SDK_CALL sdk_malloc(size_t size);
SDK_API const char *SDK_CALL sdk_error_name(int32_t code);
SDK_API const char *SDK_CALL sdk_error_string(int32_t code);
SDK_API const char *SDK_CALL sdk_event_name(int32_t evtid);
SDK_API bool SDK_CALL sdk_is_error(int32_t code);
SDK_API bool SDK_CALL sdk_is_warn(int32_t code);
SDK_API bool SDK_CALL sdk_is_reqid(int32_t code);
SDK_API bool SDK_CALL sdk_is_ok(int32_t code);
SDK_API bool SDK_CALL sdk_is_done(int32_t code);
SDK_API bool SDK_CALL sdk_is_event(int32_t code);

#ifdef __cplusplus
}
#endif

/* C++ interface (kept in the same header but only visible to C++ callers) */
#ifdef __cplusplus
class ISDK {
public:
  virtual ~ISDK() = default;
  virtual int32_t Info(char **, size_t *) const = 0;
  virtual int32_t UpdateToken(const char *data, size_t len) const = 0;
  virtual int32_t CreateEnv(const char *data, size_t len) const = 0;
  virtual int32_t DestroyEnv(const char *data, size_t len) const = 0;
  virtual int32_t PageEnv(const char *data, size_t len) const = 0;
  virtual int32_t UpdateEnv(const char *data, size_t len) const = 0;
  virtual int32_t BrowserOpen(const char *data, size_t len) const = 0;
  virtual int32_t BrowserClose(const char *data, size_t len) const = 0;
  virtual int32_t BrowserInfo(char **, size_t *) const = 0;
  virtual int32_t RegisterResultCb(sdk_result_cb_t cb, void *user_data) = 0;
  virtual int32_t RegisterCookiesStorageCb(sdk_cookies_storage_cb_t cb,
                                           void *user_data) = 0;
  virtual int32_t Shutdown() const = 0;
};
#endif

#endif ///__BROSDK_H__