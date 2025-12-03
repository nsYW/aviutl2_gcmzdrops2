#pragma once

#include <ovbase.h>

/**
 * @brief Function pointer type for worker thread execution callbacks
 */
typedef void (*gcmz_do_sub_func)(void *data);

/**
 * @brief Initialize worker thread execution system
 *
 * Creates a single dedicated worker thread for executing tasks.
 * Only one task can execute at a time. If a task is already running,
 * gcmz_do_sub() will block until the current task completes.
 *
 * @param err [out] Error information on failure
 * @return true on success, false on failure
 */
NODISCARD bool gcmz_do_sub_init(struct ov_error *const err);

/**
 * @brief Terminate the worker thread execution system
 *
 * Waits for any currently running task to complete before shutting down.
 * After this call, the worker thread will be destroyed.
 */
void gcmz_do_sub_exit(void);

/**
 * @brief Execute function on the worker thread asynchronously
 *
 * Posts a task to the worker thread. If a task is already running,
 * this function will BLOCK until the current task completes.
 * This ensures only one task runs at a time without queuing.
 *
 * @param func Function pointer to execute (must not be NULL)
 * @param data Data to pass to the function (can be NULL)
 */
void gcmz_do_sub(gcmz_do_sub_func func, void *data);

/**
 * @brief Execute function on the worker thread and wait for completion
 *
 * Posts a task to the worker thread and blocks the caller until
 * the task completes execution. If a task is already running,
 * this function will wait for both the current task and the posted task.
 *
 * @param func Function pointer to execute (must not be NULL)
 * @param data Data to pass to the function (can be NULL)
 */
void gcmz_do_sub_blocking(gcmz_do_sub_func func, void *data);
