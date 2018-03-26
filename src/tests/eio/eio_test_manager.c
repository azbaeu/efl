#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <Ecore.h>
#include <Ecore_File.h>
#include <Eio.h>

#include "eio_suite.h"
#include "eio_test_common.h"

#ifndef O_BINARY
# define O_BINARY 0
#endif

static uint64_t test_count = 0;
static Eina_Bool direct = EINA_FALSE;

#define DONE_CALLED 0xdeadbeef

static void
_access_cb(void *data, Eina_Accessor *access)
{
   uint64_t *number_of_listed_files = data;
   Eina_Stringshare *s;
   unsigned int count;

   EINA_ACCESSOR_FOREACH(access, count, s)
     {
        (*number_of_listed_files)++;
     }
}

static void
_progress_cb(void *data, const Efl_Event *ev)
{
   Efl_Future_Event_Progress *p = ev->info;
   const Eina_Array *batch = p->progress;
   uint64_t *number_of_listed_files = data;

   (*number_of_listed_files) += eina_array_count(batch);
}

static Eina_Value
_future_cb(void *data,
           const Eina_Value file,
           const Eina_Future *dead EINA_UNUSED)
{
   if (file.type == EINA_VALUE_TYPE_ERROR)
     {
        Eina_Error err;

        eina_value_get(&file, &err);
        fprintf(stderr, "Something has gone wrong: %s\n", eina_error_msg_get(err));
        abort();
     }
   if (file.type == EINA_VALUE_TYPE_UINT64)
     {
        uint64_t *number_of_listed_files = data;
        uint64_t value;

        eina_value_get(&file, &value);

        fail_if((*number_of_listed_files) != test_count);
        fail_if(value != test_count);
        *number_of_listed_files = DONE_CALLED;
     }

   ecore_main_loop_quit();

   return file;
}

static void
_done_cb(void *data, const Efl_Event *ev)
{
   Efl_Future_Event_Success *success = ev->info;
   uint64_t *files_count = success->value;
   uint64_t *number_of_listed_files = data;

   fail_if((*number_of_listed_files) != test_count);
   fail_if(*files_count != test_count);
   *number_of_listed_files = DONE_CALLED;
   ecore_main_loop_quit();
}

static void
_error_cb(void *data EINA_UNUSED, const Efl_Event *ev)
{
   Efl_Future_Event_Failure *failure = ev->info;
   const char *msg = eina_error_msg_get(failure->error);

   EINA_LOG_ERR("error: %s", msg);
   ecore_main_loop_quit();
}

static Eina_Value
_open_done_cb(void *data,
              const Eina_Value file,
              const Eina_Future *dead EINA_UNUSED)
{
   if (file.type == EINA_VALUE_TYPE_ERROR)
     {
        Eina_Error err;

        eina_value_get(&file, &err);
        fprintf(stderr, "Something has gone wrong: %s\n", eina_error_msg_get(err));
        abort();
     }
   if (file.type == EINA_VALUE_TYPE_FILE)
     {
        Eina_Bool *opened = (Eina_Bool *)data;

        *opened = EINA_TRUE;
     }
   ecore_main_loop_quit();

   return file;
}

static Eina_Value
_stat_done_cb(void *data,
              const Eina_Value st,
              const Eina_Future *dead EINA_UNUSED)
{
   Eina_Bool *is_dir = data;
   unsigned int rights;

   if (st.type == EINA_VALUE_TYPE_ERROR)
     {
        Eina_Error err;
        eina_value_get(&st, &err);
        fprintf(stderr, "Something has gone wrong: %s\n", eina_error_msg_get(err));
        abort();
     }

   if (st.type == EINA_VALUE_TYPE_STRUCT)
     {
        unsigned int mode = 0;

        fail_if(!eina_value_struct_get(&st, "mode", &mode));
        fail_if(S_ISDIR(mode) != *is_dir);
        fail_if(S_ISLNK(mode));

        rights = mode & (S_IRWXU | S_IRWXG | S_IRWXO);
        fail_if(rights != default_rights);
     }

   ecore_main_loop_quit();

   return st;
}

static void
_test_ls(Efl_Future *(*func)(Eo *obj, const char *path, Eina_Bool recursive),
         uint64_t expected_test_count,
         const char* test_dirname)
{
   Efl_Io_Manager *job = efl_add(EFL_IO_MANAGER_CLASS, efl_main_loop_get());
   Efl_Future *f = NULL;
   uint64_t main_files = 0;

   fail_if(!job);

   f = func(job, test_dirname, EINA_FALSE);
   fail_if(!f);
   test_count = expected_test_count;
   efl_future_then(f, &_done_cb, &_error_cb, &_progress_cb, &main_files);

   ecore_main_loop_begin();

   fail_if(main_files != DONE_CALLED);
   main_files = 0;

   f = func(job, test_dirname, EINA_TRUE);
   test_count = expected_test_count + 4;
   efl_future_then(f, &_done_cb, &_error_cb, &_progress_cb, &main_files);

   ecore_main_loop_begin();

   fail_if(main_files != DONE_CALLED);

   efl_del(job);
}

EFL_START_TEST(efl_io_manager_test_stat)
{
   Eina_Tmpstr *test_dirname;
   Eina_Tmpstr *nested_dirname;
   Eina_Tmpstr *nested_filename;
   Efl_Io_Manager *job;
   Eina_Future *f;
   Eina_Bool is_dir = EINA_TRUE;
   int ret;

   ret = ecore_init();
   fail_if(ret < 1);
   ret = eio_init();
   fail_if(ret < 1);
   ret = eina_init();
   fail_if(ret < 1);
   ret = ecore_file_init();
   fail_if(ret < 1);

   test_dirname = get_eio_test_file_tmp_dir();
   nested_dirname = create_test_dirs(test_dirname);
   nested_filename = get_full_path(test_dirname, files[3]);

   job = efl_add(EFL_IO_MANAGER_CLASS, efl_main_loop_get());
   fail_if(!job);

   // Start testing
   f = efl_io_manager_stat(job, nested_dirname);
   eina_future_then(f, _stat_done_cb, &is_dir);
   ecore_main_loop_begin();

   is_dir = EINA_FALSE;
   f = efl_io_manager_stat(job, nested_filename);
   eina_future_then(f, _stat_done_cb, &is_dir);
   ecore_main_loop_begin();

   // Cleanup
   efl_del(job);
   fail_if(!ecore_file_recursive_rm(test_dirname));

   eina_tmpstr_del(nested_dirname);
   eina_tmpstr_del(test_dirname);
   eina_tmpstr_del(nested_filename);
   ecore_file_shutdown();
   eina_shutdown();
   eio_shutdown();
   ecore_shutdown();
}
EFL_END_TEST

EFL_START_TEST(efl_io_manager_test_ls)
{
   Eina_Tmpstr *test_dirname;
   Eina_Tmpstr *nested_dirname;
   Eina_Tmpstr *nested_filename;
   Efl_Io_Manager *job;
   Eina_Future *f;
   uint64_t main_files = 0;
   int ret;

   ret = ecore_init();
   fail_if(ret < 1);
   ret = eio_init();
   fail_if(ret < 1);
   ret = eina_init();
   fail_if(ret < 1);
   ret = ecore_file_init();
   fail_if(ret < 1);

   test_dirname = get_eio_test_file_tmp_dir();
   nested_dirname = create_test_dirs(test_dirname);
   nested_filename = get_full_path(test_dirname, files[3]);

   // Start testing
   job = efl_add(EFL_IO_MANAGER_CLASS, efl_main_loop_get());
   fail_if(!job);

   f = efl_io_manager_ls(job, test_dirname, &main_files, _access_cb, NULL);
   test_count = 6;
   eina_future_then(f, _future_cb, &main_files);

   ecore_main_loop_begin();

   fail_if(main_files != DONE_CALLED);

   // No recursion for efl_io_manager_ls, should I fix that ?
   /* _test_ls(&efl_io_manager_ls, 5, test_dirname); */
   direct = EINA_TRUE;
   _test_ls(&efl_io_manager_stat_ls, 6, test_dirname);
   _test_ls(&efl_io_manager_direct_ls, 6, test_dirname);

   // Cleanup
   efl_del(job);
   fail_if(!ecore_file_recursive_rm(test_dirname));

   eina_tmpstr_del(nested_dirname);
   eina_tmpstr_del(test_dirname);
   eina_tmpstr_del(nested_filename);
   ecore_file_shutdown();
   eina_shutdown();
   eio_shutdown();
   ecore_shutdown();
}
EFL_END_TEST

EFL_START_TEST(efl_io_manager_test_open)
{
   Eina_Tmpstr *test_dirname;
   Eina_Tmpstr *nested_dirname;
   Eina_Tmpstr *nested_filename;
   Efl_Io_Manager *job;
   Eina_Future *f;
   Eina_Bool opened_file = EINA_FALSE;
   int ret;

   ret = ecore_init();
   fail_if(ret < 1);
   ret = eina_init();
   fail_if(ret < 1);
   ret = ecore_file_init();
   fail_if(ret < 1);
   ret = eio_init();
   fail_if(ret < 1);

   test_dirname = get_eio_test_file_tmp_dir();
   nested_dirname = create_test_dirs(test_dirname);
   nested_filename = get_full_path(test_dirname, files[3]);

   job = efl_add(EFL_IO_MANAGER_CLASS, efl_main_loop_get());

   f = efl_io_manager_open(job, nested_filename, EINA_FALSE);
   eina_future_then(f, _open_done_cb, &opened_file);
   ecore_main_loop_begin();

   fail_if(!opened_file);

   // Cleanup
   efl_del(job);
   fail_if(!ecore_file_recursive_rm(test_dirname));

   eina_tmpstr_del(nested_dirname);
   eina_tmpstr_del(test_dirname);

   eio_shutdown();
   eina_tmpstr_del(nested_filename);
   ecore_file_shutdown();
   eina_shutdown();
   ecore_shutdown();
}
EFL_END_TEST

EFL_START_TEST(efl_io_instantiated)
{
   Efl_Io_Manager *manager;

   ecore_init();

   fail_if(efl_provider_find(efl_main_loop_get(), EFL_IO_MANAGER_CLASS) != NULL);

   eio_init();

   manager = efl_provider_find(efl_main_loop_get(), EFL_IO_MANAGER_CLASS);
   fail_if(manager == NULL);
   fail_if(!efl_isa(manager, EFL_IO_MANAGER_CLASS));

   eio_shutdown();

   ecore_shutdown();
}
EFL_END_TEST

void
eio_test_job(TCase *tc)
{
    tcase_add_test(tc, efl_io_manager_test_ls);
    tcase_add_test(tc, efl_io_manager_test_stat);
    tcase_add_test(tc, efl_io_manager_test_open);
    tcase_add_test(tc, efl_io_instantiated);
}
