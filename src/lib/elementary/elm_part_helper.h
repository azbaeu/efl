#ifndef _ELM_PART_HELPER_H
#define _ELM_PART_HELPER_H

#include "efl_ui_layout_part_legacy.eo.h"

//#define ELM_PART_HOOK do { ERR("%s@%p:%s [%d]", efl_class_name_get(pd->obj), pd->obj, pd->part, (int) pd->temp); } while(0)
#define ELM_PART_HOOK do {} while(0)

//#define ELM_PART_REF(obj, pd) do { if (!(pd->temp++)) efl_ref(obj); } while(0)
#define ELM_PART_UNREF(obj, pd) do { if (pd->temp) { if (!(--pd->temp)) efl_del(obj); } } while(0)
#define ELM_PART_RETURN_VAL(a) do { ELM_PART_HOOK; typeof(a) _ret = a; ELM_PART_UNREF(obj, pd); return _ret; } while(0)
#define ELM_PART_RETURN_VOID do { ELM_PART_HOOK; ELM_PART_UNREF(obj, pd); return; } while(0)
//#define ELM_PART_CALL(a) ({ ELM_PART_REF(obj, pd); a; })

typedef struct _Elm_Part_Data Elm_Part_Data;
struct _Elm_Part_Data
{
   Eo             *obj;
   Eina_Tmpstr    *part;
   unsigned char   temp;
};

// Note: this generic implementation can be improved to support part object
// caching or something...


#define ELM_PART_CONTENT_DEFAULT_SET(type, part) \
   static const char * _ ## type ## _default_content_part_get(const Eo *obj EINA_UNUSED, void *sd EINA_UNUSED) { return part; }

#define ELM_PART_CONTENT_DEFAULT_OPS(type) \
   EFL_OBJECT_OP_FUNC(elm_widget_default_content_part_get, _ ## type ## _default_content_part_get)

#define ELM_PART_TEXT_DEFAULT_GET(type, part) \
   static const char * _ ## type ## _default_text_part_get(const Eo *obj EINA_UNUSED, void *sd EINA_UNUSED) { return part; }

#define ELM_PART_TEXT_DEFAULT_OPS(type) \
   EFL_OBJECT_OP_FUNC(elm_widget_default_text_part_get, _ ## type ## _default_text_part_get)


// For any widget that has specific part handling

static inline Eina_Bool
_elm_part_alias_find(const Elm_Layout_Part_Alias_Description *aliases, const char *part)
{
   const Elm_Layout_Part_Alias_Description *alias;

   for (alias = aliases; alias && alias->alias; alias++)
     if (eina_streq(alias->real_part, part))
       return EINA_TRUE;
   return EINA_FALSE;
}

#define ELM_PART_OVERRIDE_IMPLEMENT(PART_CLASS) ({ \
   Eo *proxy = efl_add(PART_CLASS, (Eo *) obj); \
   Elm_Part_Data *pd = efl_data_scope_get(proxy, EFL_UI_WIDGET_PART_CLASS); \
   if (pd) \
     { \
        pd->obj = (Eo *) obj; \
        pd->part = eina_tmpstr_add(part); \
        pd->temp = 1; \
     } \
   proxy; })

#define ELM_PART_OVERRIDE_ONLY_ALIASES(type, TYPE, typedata, aliases) \
   EOLIAN static Efl_Object * \
   _ ## type ## _efl_part_part(const Eo *obj, typedata *priv EINA_UNUSED, const char *part) \
   { \
      EINA_SAFETY_ON_NULL_RETURN_VAL(part, NULL); \
      if (_elm_part_alias_find(aliases, part)) \
        return ELM_PART_OVERRIDE_IMPLEMENT(TYPE ## _PART_CLASS); \
      return efl_part(efl_super(obj, MY_CLASS), part); \
   }

#define ELM_PART_OVERRIDE(type, TYPE, typedata) \
EOLIAN static Efl_Object * \
_ ## type ## _efl_part_part(const Eo *obj, typedata *priv EINA_UNUSED, const char *part) \
{ \
   EINA_SAFETY_ON_NULL_RETURN_VAL(part, NULL); \
   return ELM_PART_OVERRIDE_IMPLEMENT(TYPE ## _PART_CLASS); \
}

#define ELM_PART_OVERRIDE_CONTENT_SET_FULL(full, type, TYPE, typedata) \
EOLIAN static Eina_Bool \
_ ## full ## _efl_container_content_set(Eo *obj, void *_pd EINA_UNUSED, Efl_Gfx *content) \
{ \
   Elm_Part_Data *pd = efl_data_scope_get(obj, EFL_UI_WIDGET_PART_CLASS); \
   typedata *sd = efl_data_scope_get(pd->obj, TYPE ## _CLASS); \
   ELM_PART_RETURN_VAL(_ ## type ## _content_set(pd->obj, sd, pd->part, content)); \
}

#define ELM_PART_OVERRIDE_CONTENT_GET_FULL(full, type, TYPE, typedata) \
EOLIAN static Efl_Gfx * \
_ ## full ## _efl_container_content_get(Eo *obj, void *_pd EINA_UNUSED) \
{ \
   Elm_Part_Data *pd = efl_data_scope_get(obj, EFL_UI_WIDGET_PART_CLASS); \
   typedata *sd = efl_data_scope_get(pd->obj, TYPE ## _CLASS); \
   ELM_PART_RETURN_VAL(_ ## type ## _content_get(pd->obj, sd, pd->part)); \
}

#define ELM_PART_OVERRIDE_CONTENT_UNSET_FULL(full, type, TYPE, typedata) \
EOLIAN static Efl_Gfx * \
_ ## full ## _efl_container_content_unset(Eo *obj, void *_pd EINA_UNUSED) \
{ \
   Elm_Part_Data *pd = efl_data_scope_get(obj, EFL_UI_WIDGET_PART_CLASS); \
   typedata *sd = efl_data_scope_get(pd->obj, TYPE ## _CLASS); \
   ELM_PART_RETURN_VAL(_ ## type ## _content_unset(pd->obj, sd, pd->part)); \
}

#define ELM_PART_OVERRIDE_TEXT_SET_FULL(full, type, TYPE, typedata) \
EOLIAN static void \
_ ## full ## _efl_text_text_set(Eo *obj, void *_pd EINA_UNUSED, const char *text) \
{ \
   Elm_Part_Data *pd = efl_data_scope_get(obj, EFL_UI_WIDGET_PART_CLASS); \
   typedata *sd = efl_data_scope_get(pd->obj, TYPE ## _CLASS); \
   _ ## type ## _text_set(pd->obj, sd, pd->part, text); \
   ELM_PART_RETURN_VOID; \
}

#define ELM_PART_OVERRIDE_TEXT_GET_FULL(full, type, TYPE, typedata) \
EOLIAN static const char *\
_ ## full ## _efl_text_text_get(Eo *obj, void *_pd EINA_UNUSED) \
{ \
   Elm_Part_Data *pd = efl_data_scope_get(obj, EFL_UI_WIDGET_PART_CLASS); \
   typedata *sd = efl_data_scope_get(pd->obj, TYPE ## _CLASS); \
   ELM_PART_RETURN_VAL(_ ## type ## _text_get(pd->obj, sd, pd->part)); \
}

#define ELM_PART_OVERRIDE_TEXT_MARKUP_GET_FULL(full, type, TYPE, typedata) \
EOLIAN static const char *\
_ ## full ## _efl_text_markup_markup_get(Eo *obj, void *_pd EINA_UNUSED) \
{ \
   Elm_Part_Data *pd = efl_data_scope_get(obj, EFL_UI_WIDGET_PART_CLASS); \
   typedata *sd = efl_data_scope_get(pd->obj, TYPE ## _CLASS); \
   ELM_PART_RETURN_VAL(_ ## type ## _text_markup_get(pd->obj, sd, pd->part)); \
}

#define ELM_PART_OVERRIDE_TEXT_MARKUP_SET_FULL(full, type, TYPE, typedata) \
EOLIAN static void \
_ ## full ## _efl_text_markup_markup_set(Eo *obj, void *_pd EINA_UNUSED, const char *markup) \
{ \
   Elm_Part_Data *pd = efl_data_scope_get(obj, EFL_UI_WIDGET_PART_CLASS); \
   typedata *sd = efl_data_scope_get(pd->obj, TYPE ## _CLASS); \
   _ ## type ## _text_markup_set(pd->obj, sd, pd->part, markup); \
  ELM_PART_RETURN_VOID; \
}

#define ELM_PART_OVERRIDE_CONTENT_SET(type, TYPE, typedata) \
   ELM_PART_OVERRIDE_CONTENT_SET_FULL(type ## _part, type, TYPE, typedata)

#define ELM_PART_OVERRIDE_CONTENT_GET(type, TYPE, typedata) \
   ELM_PART_OVERRIDE_CONTENT_GET_FULL(type ## _part, type, TYPE, typedata)

#define ELM_PART_OVERRIDE_CONTENT_UNSET(type, TYPE, typedata) \
   ELM_PART_OVERRIDE_CONTENT_UNSET_FULL(type ## _part, type, TYPE, typedata)

#define ELM_PART_OVERRIDE_TEXT_SET(type, TYPE, typedata) \
   ELM_PART_OVERRIDE_TEXT_SET_FULL(type ## _part, type, TYPE, typedata)

#define ELM_PART_OVERRIDE_TEXT_GET(type, TYPE, typedata) \
   ELM_PART_OVERRIDE_TEXT_GET_FULL(type ## _part, type, TYPE, typedata)

#define ELM_PART_OVERRIDE_MARKUP_SET(type, TYPE, typedata) \
   ELM_PART_OVERRIDE_TEXT_MARKUP_SET_FULL(type ## _part, type, TYPE, typedata)

#define ELM_PART_OVERRIDE_MARKUP_GET(type, TYPE, typedata) \
   ELM_PART_OVERRIDE_TEXT_MARKUP_GET_FULL(type ## _part, type, TYPE, typedata)

#define ELM_PART_TEXT_DEFAULT_IMPLEMENT(type, Type) \
EOLIAN static void \
_ ## type ## _efl_text_text_set(Eo *obj, Type *pd EINA_UNUSED, const char *text) \
{ \
   efl_text_set(efl_part(efl_super(obj, MY_CLASS), "elm.text"), text); \
} \
EOLIAN static const char * \
_ ## type ## _efl_text_text_get(Eo *obj, Type *pd EINA_UNUSED) \
{ \
  return efl_text_get(efl_part(efl_super(obj, MY_CLASS), "elm.text")); \
} \
EOLIAN static const char * \
_ ## type ## _efl_text_markup_markup_get(Eo *obj, Type *pd EINA_UNUSED) \
{ \
  return efl_text_markup_get(efl_part(efl_super(obj, MY_CLASS), "elm.text")); \
} \
EOLIAN static void \
_ ## type ## _efl_text_markup_markup_set(Eo *obj, Type *pd EINA_UNUSED, const char *markup) \
{ \
  return efl_text_markup_set(efl_part(efl_super(obj, MY_CLASS), "elm.text"), markup); \
} \
EOLIAN static void \
_ ## type ## _efl_ui_translatable_translatable_text_set(Eo *obj, Type *pd EINA_UNUSED, const char *label, const char *domain) \
{ \
   efl_ui_translatable_text_set(efl_part(efl_super(obj, MY_CLASS), "elm.text"), label, domain); \
} \
EOLIAN static const char * \
_ ## type ## _efl_ui_translatable_translatable_text_get(Eo *obj, Type *pd EINA_UNUSED, const char **domain) \
{ \
  return efl_ui_translatable_text_get(efl_part(efl_super(obj, MY_CLASS), "elm.text"), domain); \
}

#endif
