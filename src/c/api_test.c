#include <ovtest.h>

#include <ovarray.h>
#include <ovprintf.h>

#include <string.h>

#ifdef _WIN32
#  include <windows.h>
#endif

#include "api.h"
#include "file.h"
#include "gcmz_types.h"

#ifndef SOURCE_DIR
#  define SOURCE_DIR .
#endif

// Shared memory structure for reading API window handle
struct gcmzdrops_fmo_test {
  uint32_t window;
  int32_t width;
  int32_t height;
  int32_t video_rate;
  int32_t video_scale;
  int32_t audio_rate;
  int32_t audio_ch;
  int32_t gcmz_api_ver;
  wchar_t project_path[260];
  uint32_t flags;
  uint32_t aviutl2_ver;
  uint32_t gcmz_ver;
};

// Test request callback context
struct test_request_context {
  bool callback_called;
  struct gcmz_file_list *received_files;
  int received_layer;
  int received_frame_advance;
  int received_margin;
  bool received_use_exo_converter;
  struct ov_error *received_error;
  void *received_userdata;
};

// Test callback function that stores received parameters
static void test_request_callback(struct gcmz_api_request_params *const params,
                                  gcmz_api_request_complete_func const complete) {
  struct test_request_context *ctx = (struct test_request_context *)params->userdata;
  if (ctx) {
    ctx->callback_called = true;
    ctx->received_files = params->files;
    ctx->received_layer = params->layer;
    ctx->received_frame_advance = params->frame_advance;
    ctx->received_margin = params->margin;
    ctx->received_use_exo_converter = params->use_exo_converter;
    ctx->received_error = params->err;
    ctx->received_userdata = params->userdata;
  }
  if (complete) {
    complete(params);
  }
}

// Test callback for project data update notification
static void test_notify_update(struct gcmz_api *const api, void *const userdata) {
  (void)userdata;
  if (!api) {
    return;
  }
  struct gcmz_project_data data = {
      .width = 1920,
      .height = 1080,
      .video_rate = 30,
      .video_scale = 1,
      .sample_rate = 48000,
      .audio_ch = 2,
      .project_path = L"C:\\test\\project.aup",
  };
  struct ov_error err = {0};
  TEST_SUCCEEDED(gcmz_api_set_project_data(api, &data, &err), &err);
}

// Check if GCMZDrops mutex already exists (indicates another instance is running)
static bool gcmzdrops_mutex_exists(void) {
  HANDLE mutex = OpenMutexW(SYNCHRONIZE, FALSE, L"GCMZDropsMutex");
  if (mutex) {
    CloseHandle(mutex);
    return true;
  }
  return false;
}

// Skip test if mutex already exists
#define SKIP_IF_MUTEX_EXISTS()                                                                                         \
  do {                                                                                                                 \
    if (gcmzdrops_mutex_exists()) {                                                                                    \
      TEST_SKIP("\"GCMZDropsMutex\" mutex already exists in environment");                                             \
      return;                                                                                                          \
    }                                                                                                                  \
  } while (0)

// Default project data for testing
static struct gcmz_project_data const g_default_project_data = {
    .width = 1920,
    .height = 1080,
    .video_rate = 30,
    .video_scale = 1,
    .sample_rate = 48000,
    .audio_ch = 2,
    .cursor_frame = 0,
    .display_frame = 0,
    .display_layer = 1,
    .display_zoom = 100,
    .flags = 0,
    .project_path = L"C:\\test\\project.aup",
};

// Test API fixture for WM_COPYDATA tests
struct test_api_fixture {
  struct gcmz_api *api;
  struct test_request_context ctx;
  HWND hwnd;
};

// Initialize test fixture: create API, set project data, get window handle
static bool test_api_fixture_init(struct test_api_fixture *const fixture, struct ov_error *const err) {
  if (!fixture) {
    OV_ERROR_SET_GENERIC(err, ov_error_generic_invalid_argument);
    return false;
  }

  *fixture = (struct test_api_fixture){0};

  fixture->api = gcmz_api_create(
      &(struct gcmz_api_options){
          .request_callback = test_request_callback,
          .update_callback = NULL,
          .userdata = &fixture->ctx,
      },
      err);
  if (!fixture->api) {
    OV_ERROR_ADD_TRACE(err);
    return false;
  }

  if (!gcmz_api_set_project_data(fixture->api, &g_default_project_data, err)) {
    OV_ERROR_ADD_TRACE(err);
    gcmz_api_destroy(&fixture->api);
    return false;
  }

  // Get window handle from shared memory
  HANDLE fmo = OpenFileMappingW(FILE_MAP_READ, FALSE, L"GCMZDrops");
  if (!fmo) {
    OV_ERROR_SET_HRESULT(err, HRESULT_FROM_WIN32(GetLastError()));
    gcmz_api_destroy(&fixture->api);
    return false;
  }

  struct gcmzdrops_fmo_test *shared =
      (struct gcmzdrops_fmo_test *)MapViewOfFile(fmo, FILE_MAP_READ, 0, 0, sizeof(struct gcmzdrops_fmo_test));
  if (!shared) {
    OV_ERROR_SET_HRESULT(err, HRESULT_FROM_WIN32(GetLastError()));
    CloseHandle(fmo);
    gcmz_api_destroy(&fixture->api);
    return false;
  }

  fixture->hwnd = (HWND)(uintptr_t)shared->window;
  UnmapViewOfFile(shared);
  CloseHandle(fmo);

  if (!fixture->hwnd) {
    OV_ERROR_SET(err, ov_error_type_generic, ov_error_generic_fail, "window handle is NULL");
    gcmz_api_destroy(&fixture->api);
    return false;
  }

  return true;
}

// Cleanup test fixture
static void test_api_fixture_cleanup(struct test_api_fixture *const fixture) {
  if (fixture && fixture->api) {
    gcmz_api_destroy(&fixture->api);
  }
}

// API format versions for external API
enum {
  api_format_v0 = 0, // Legacy non-JSON format
  api_format_v1 = 1, // JSON format with EXO conversion
  api_format_v2 = 2, // JSON format without EXO conversion
};

// Send WM_COPYDATA request and verify callback was invoked
static bool test_api_send_request(struct test_api_fixture *const fixture,
                                  int const format_version,
                                  char *const json_data,
                                  size_t const json_len) {
  if (!fixture || !fixture->hwnd || !json_data) {
    return false;
  }

  fixture->ctx.callback_called = false;
  fixture->ctx.received_margin = 999; // Sentinel value

  COPYDATASTRUCT cds = {
      .dwData = (ULONG_PTR)format_version,
      .cbData = (DWORD)json_len,
      .lpData = json_data,
  };

  LRESULT result = SendMessageW(fixture->hwnd, WM_COPYDATA, 0, (LPARAM)&cds);
  if (result != TRUE) {
    return false;
  }

  return fixture->ctx.callback_called;
}

static void test_api_create_destroy(void) {
  SKIP_IF_MUTEX_EXISTS();

  struct gcmz_api *api = NULL;
  struct ov_error err = {0};

  // Test successful creation with options
  api = gcmz_api_create(
      &(struct gcmz_api_options){
          .request_callback = NULL,
          .update_callback = test_notify_update,
          .userdata = NULL,
      },
      &err);
  if (!TEST_SUCCEEDED(api != NULL, &err)) {
    return;
  }

  gcmz_api_destroy(&api);
  TEST_ASSERT(api == NULL);

  // Test destroy with NULL (should not crash)
  gcmz_api_destroy(NULL);

  // Test create with NULL options
  api = gcmz_api_create(NULL, NULL);
  if (api) {
    gcmz_api_destroy(&api);
  }
}

static void test_api_callback_setting(void) {
  SKIP_IF_MUTEX_EXISTS();

  struct gcmz_api *api = NULL;
  struct ov_error err = {0};
  struct test_request_context ctx = {0};

  api = gcmz_api_create(
      &(struct gcmz_api_options){
          .request_callback = test_request_callback,
          .update_callback = test_notify_update,
          .userdata = &ctx,
      },
      &err);
  if (!TEST_SUCCEEDED(api != NULL, &err)) {
    return;
  }

  gcmz_api_destroy(&api);
}

static void test_thread_management(void) {
  SKIP_IF_MUTEX_EXISTS();

  struct gcmz_api *api = NULL;
  struct ov_error err = {0};
  struct test_request_context ctx = {0};

  api = gcmz_api_create(
      &(struct gcmz_api_options){
          .request_callback = test_request_callback,
          .update_callback = test_notify_update,
          .userdata = &ctx,
      },
      &err);
  if (!TEST_SUCCEEDED(api != NULL, &err)) {
    return;
  }

  gcmz_api_destroy(&api);
  TEST_ASSERT(api == NULL);
}

static void test_file_list_operations(void) {
  struct gcmz_file_list *list = NULL;
  struct ov_error err = {0};

  list = gcmz_file_list_create(&err);
  if (!TEST_SUCCEEDED(list != NULL, &err)) {
    return;
  }

  // Test initial state
  TEST_CHECK(gcmz_file_list_count(list) == 0);

  // Test adding regular file
  TEST_SUCCEEDED(gcmz_file_list_add(list, L"C:\\test\\file1.txt", L"text/plain", &err), &err);
  TEST_CHECK(gcmz_file_list_count(list) == 1);

  struct gcmz_file const *file = gcmz_file_list_get(list, 0);
  TEST_CHECK(file != NULL);
  if (file) {
    TEST_CHECK(wcscmp(file->path, L"C:\\test\\file1.txt") == 0);
    TEST_CHECK(wcscmp(file->mime_type, L"text/plain") == 0);
    TEST_CHECK(!file->temporary);
  }

  // Test adding temporary file
  TEST_SUCCEEDED(gcmz_file_list_add_temporary(list, L"C:\\temp\\temp1.tmp", L"application/octet-stream", &err), &err);
  TEST_CHECK(gcmz_file_list_count(list) == 2);

  struct gcmz_file const *temp_file = gcmz_file_list_get(list, 1);
  TEST_CHECK(temp_file != NULL);
  if (temp_file) {
    TEST_CHECK(wcscmp(temp_file->path, L"C:\\temp\\temp1.tmp") == 0);
    TEST_CHECK(temp_file->temporary);
  }

  // Test removing file
  TEST_SUCCEEDED(gcmz_file_list_remove(list, 0, &err), &err);
  TEST_CHECK(gcmz_file_list_count(list) == 1);

  // Test out of bounds access
  TEST_CHECK(gcmz_file_list_get(list, 10) == NULL);

  gcmz_file_list_destroy(&list);
  TEST_CHECK(list == NULL);

  // Test NULL safety
  TEST_CHECK(gcmz_file_list_count(NULL) == 0);
  TEST_CHECK(gcmz_file_list_get(NULL, 0) == NULL);
  gcmz_file_list_destroy(NULL);
}

static void test_security_validation(void) {
  struct gcmz_file_list *list = NULL;
  struct ov_error err = {0};

  list = gcmz_file_list_create(&err);
  if (!TEST_SUCCEEDED(list != NULL, &err)) {
    return;
  }

  TEST_SUCCEEDED(gcmz_file_list_add(list, L"C:\\test\\normalfile.txt", L"text/plain", &err), &err);
  gcmz_file_list_destroy(&list);
}

static void test_error_handling(void) {
  struct ov_error err = {0};

  // Test API creation with NULL err
  struct gcmz_api *null_api = gcmz_api_create(NULL, NULL);
  if (null_api) {
    gcmz_api_destroy(&null_api);
  }

  // Test file list creation with NULL err
  struct gcmz_file_list *list = gcmz_file_list_create(NULL);
  if (list) {
    gcmz_file_list_destroy(&list);
  }

  // Test adding file with NULL path
  list = gcmz_file_list_create(&err);
  if (!TEST_SUCCEEDED(list != NULL, &err)) {
    return;
  }
  TEST_FAILED_WITH(gcmz_file_list_add(list, NULL, L"text/plain", &err),
                   &err,
                   ov_error_type_generic,
                   ov_error_generic_invalid_argument);

  gcmz_file_list_destroy(&list);
}

static void test_api_structures(void) {
  struct gcmz_api_request_params params = {0};
  struct ov_error err = {0};

  params.files = gcmz_file_list_create(&err);
  if (!TEST_SUCCEEDED(params.files != NULL, &err)) {
    return;
  }

  params.layer = 1;
  params.frame_advance = 0;
  params.margin = -1;

  TEST_CHECK(params.layer == 1);
  TEST_CHECK(params.frame_advance == 0);
  TEST_CHECK(params.margin == -1);
  TEST_CHECK(gcmz_file_list_count(params.files) == 0);

  gcmz_file_list_destroy(&params.files);
}

static void test_multiple_data_sets(void) {
  SKIP_IF_MUTEX_EXISTS();

  struct gcmz_api *api = NULL;
  struct ov_error err = {0};

  api = gcmz_api_create(
      &(struct gcmz_api_options){
          .update_callback = test_notify_update,
      },
      &err);
  if (!TEST_SUCCEEDED(api != NULL, &err)) {
    return;
  }

  // Test various project data configurations
  struct {
    int width;
    int height;
    wchar_t *path;
  } const test_cases[] = {
      {1920, 1080, L"C:\\first\\project\\path.aup"},
      {1280, 720, L"C:\\very\\long\\second\\project\\path\\with\\many\\subdirectories\\test.aup"},
      {640, 480, L"C:\\short.aup"},
      {800, 600, NULL},
      {1920, 1080, L"C:\\final\\project.aup"},
  };

  for (size_t i = 0; i < sizeof(test_cases) / sizeof(test_cases[0]); ++i) {
    struct gcmz_project_data data = {
        .width = test_cases[i].width,
        .height = test_cases[i].height,
        .video_rate = 30,
        .video_scale = 1,
        .sample_rate = 48000,
        .audio_ch = 2,
        .project_path = test_cases[i].path,
    };
    TEST_SUCCEEDED(gcmz_api_set_project_data(api, &data, &err), &err);
  }

  gcmz_api_destroy(&api);
}

static void test_margin_parameter(void) {
  SKIP_IF_MUTEX_EXISTS();

  struct test_api_fixture fixture = {0};
  struct ov_error err = {0};

  if (!TEST_SUCCEEDED(test_api_fixture_init(&fixture, &err), &err)) {
    return;
  }

  // Test case 1: v1 format ignores margin parameter
  {
    char json[] = "{\"layer\":2,\"frameAdvance\":5,\"margin\":10,\"files\":[\"C:\\\\test\\\\file1.txt\"]}";
    TEST_CHECK(test_api_send_request(&fixture, api_format_v1, json, strlen(json)));
    TEST_CHECK(fixture.ctx.received_layer == 2);
    TEST_CHECK(fixture.ctx.received_frame_advance == 5);
    TEST_CHECK(fixture.ctx.received_margin == -1); // Ignored in v1
    TEST_CHECK(fixture.ctx.received_use_exo_converter == true);
  }

  // Test case 2: v2 format accepts margin parameter
  {
    char json[] = "{\"layer\":3,\"frameAdvance\":7,\"margin\":15,\"files\":[\"C:\\\\test\\\\file2.txt\"]}";
    TEST_CHECK(test_api_send_request(&fixture, api_format_v2, json, strlen(json)));
    TEST_CHECK(fixture.ctx.received_layer == 3);
    TEST_CHECK(fixture.ctx.received_frame_advance == 7);
    TEST_CHECK(fixture.ctx.received_margin == 15); // Accepted in v2
    TEST_CHECK(fixture.ctx.received_use_exo_converter == false);
  }

  // Test case 3: v2 format without margin defaults to -1
  {
    char json[] = "{\"layer\":4,\"frameAdvance\":9,\"files\":[\"C:\\\\test\\\\file3.txt\"]}";
    TEST_CHECK(test_api_send_request(&fixture, api_format_v2, json, strlen(json)));
    TEST_CHECK(fixture.ctx.received_layer == 4);
    TEST_CHECK(fixture.ctx.received_frame_advance == 9);
    TEST_CHECK(fixture.ctx.received_margin == -1); // Default when omitted
  }

  test_api_fixture_cleanup(&fixture);
}

TEST_LIST = {
    {"api_create_destroy", test_api_create_destroy},
    {"api_callback_setting", test_api_callback_setting},
    {"file_list_operations", test_file_list_operations},
    {"security_validation", test_security_validation},
    {"error_handling", test_error_handling},
    {"thread_management", test_thread_management},
    {"api_structures", test_api_structures},
    {"multiple_data_sets", test_multiple_data_sets},
    {"margin_parameter", test_margin_parameter},
    {NULL, NULL},
};
