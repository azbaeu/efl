#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "evas_common_private.h"
#include "evas_private.h"

static int _evas_vg_loader_json_log_dom = -1;

#ifdef ERR
# undef ERR
#endif
#define ERR(...) EINA_LOG_DOM_ERR(_evas_vg_loader_json_log_dom, __VA_ARGS__)

#ifdef INF
# undef INF
#endif
#define INF(...) EINA_LOG_DOM_INFO(_evas_vg_loader_json_log_dom, __VA_ARGS__)

static Vg_File_Data*
evas_vg_load_file_data_json(const char *file,
                            const char *key EINA_UNUSED,
                            int *error EINA_UNUSED)
{
   return NULL;
}

static Evas_Vg_Load_Func evas_vg_load_json_func =
{
   evas_vg_load_file_data_json
};

static int
module_open(Evas_Module *em)
{
   if (!em) return 0;
   em->functions = (void *)(&evas_vg_load_json_func);
   _evas_vg_loader_json_log_dom = eina_log_domain_register
     ("vg-load-json", EVAS_DEFAULT_LOG_COLOR);
   if (_evas_vg_loader_json_log_dom < 0)
     {
        EINA_LOG_ERR("Can not create a module log domain.");
        return 0;
     }
   ERR("module open!");
   return 1;
}

static void
module_close(Evas_Module *em EINA_UNUSED)
{
   ERR("module close!");

   if (_evas_vg_loader_json_log_dom >= 0)
     {
        eina_log_domain_unregister(_evas_vg_loader_json_log_dom);
        _evas_vg_loader_json_log_dom = -1;
     }
}

static Evas_Module_Api evas_modapi =
{
   EVAS_MODULE_API_VERSION,
   "json",
   "none",
   {
     module_open,
     module_close
   }
};

EVAS_MODULE_DEFINE(EVAS_MODULE_TYPE_VG_LOADER, vg_loader, json);

#ifndef EVAS_STATIC_BUILD_VG_JSON
EVAS_EINA_MODULE_DEFINE(vg_loader, json);
#endif
