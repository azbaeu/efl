#include "main.h"
#include "docs.h"

/* Used to store the function names that will have to be appended
 * with __eolian during C generation. Needed when params have to
 * be initialized and for future features.
 */
static Eina_Hash *_funcs_params_init = NULL;

static const char *
_get_add_star(Eolian_Function_Type ftype, Eolian_Parameter_Dir pdir)
{
   if (ftype == EOLIAN_PROP_GET)
     return "*";
   if ((pdir == EOLIAN_OUT_PARAM) || (pdir == EOLIAN_INOUT_PARAM))
     return "*";
   return "";
}

static void
_gen_func(const Eolian_Class *cl, const Eolian_Function *fid,
          Eolian_Function_Type ftype, Eina_Strbuf *buf,
          const Eolian_Implement *impl)
{
   Eina_Bool is_empty = eolian_implement_is_empty(impl);
   Eina_Bool is_auto = eolian_implement_is_auto(impl);

   if ((ftype != EOLIAN_PROP_GET) && (ftype != EOLIAN_PROP_SET))
     ftype = eolian_function_type_get(fid);

   Eina_Bool is_prop = (ftype == EOLIAN_PROP_GET || ftype == EOLIAN_PROP_SET);
   Eina_Bool var_as_ret = EINA_FALSE;

   const Eolian_Expression *def_ret = NULL;
   const Eolian_Type *rtp = eolian_function_return_type_get(fid, ftype);
   if (rtp)
     {
        is_auto = EINA_FALSE; /* can't do auto if func returns */
        def_ret = eolian_function_return_default_value_get(fid, ftype);
     }

   const char *func_suffix = "";
   if (ftype == EOLIAN_PROP_GET)
     {
        func_suffix = "_get";
        if (!rtp)
          {
             void *d1, *d2;
             Eina_Iterator *itr = eolian_property_values_get(fid, ftype);
             if (eina_iterator_next(itr, &d1) && !eina_iterator_next(itr, &d2))
               {
                  Eolian_Function_Parameter *pr = d1;
                  rtp = eolian_parameter_type_get(pr);
                  var_as_ret = EINA_TRUE;
                  def_ret = eolian_parameter_default_value_get(pr);
               }
             eina_iterator_free(itr);
          }
     }
   else if (ftype == EOLIAN_PROP_SET)
     func_suffix = "_set";

   Eina_Strbuf *params = eina_strbuf_new(); /* par1, par2, par3, ... */
   Eina_Strbuf *params_full = eina_strbuf_new(); /* T par1, U par2, ... for decl */
   Eina_Strbuf *params_full_imp = eina_strbuf_new(); /* as above, for impl */
   Eina_Strbuf *params_init = eina_strbuf_new(); /* default value inits */

   Eina_Bool has_promise = EINA_FALSE;
   Eina_Stringshare *promise_param_name = NULL;
   Eina_Stringshare *promise_param_type = NULL;

   /* property keys */
   {
      Eina_Iterator *itr = eolian_property_keys_get(fid, ftype);
      Eolian_Function_Parameter *pr;
      EINA_ITERATOR_FOREACH(itr, pr)
        {
           const char *prn = eolian_parameter_name_get(pr);
           const Eolian_Type *pt = eolian_parameter_type_get(pr);
           Eina_Stringshare *ptn = eolian_type_c_type_get(pt);

           if (eina_strbuf_length_get(params))
             eina_strbuf_append(params, ", ");
           eina_strbuf_append(params, prn);

           eina_strbuf_append_printf(params_full, ", %s %s", ptn, prn);
           eina_strbuf_append_printf(params_full_imp, ", %s %s", ptn, prn);
           if (is_empty)
             eina_strbuf_append(params_full_imp, " EINA_UNUSED");

           eina_stringshare_del(ptn);
        }
      eina_iterator_free(itr);
   }

   /* property values or method params if applicable */
   if (!var_as_ret)
     {
        Eina_Iterator *itr;
        if (is_prop)
          itr = eolian_property_values_get(fid, ftype);
        else
          itr = eolian_function_parameters_get(fid);
        Eolian_Function_Parameter *pr;
        EINA_ITERATOR_FOREACH(itr, pr)
          {
             Eolian_Parameter_Dir pd = eolian_parameter_direction_get(pr);
             const Eolian_Expression *dfv = eolian_parameter_default_value_get(pr);
             const char *prn = eolian_parameter_name_get(pr);
             const Eolian_Type *pt = eolian_parameter_type_get(pr);
             Eina_Stringshare *ptn = eolian_type_c_type_get(pt);

             Eina_Bool had_star = !!strchr(ptn, '*');
             const char *add_star = _get_add_star(ftype, pd);

             if (eina_strbuf_length_get(params))
               eina_strbuf_append(params, ", ");

             /* XXX: this is really bad */
             if (!has_promise && !strcmp(ptn, "Eina_Promise *") && !is_prop
                 && (pd == EOLIAN_INOUT_PARAM))
               {
                  has_promise = EINA_TRUE;
                  promise_param_name = eina_stringshare_add(prn);
                  promise_param_type = eolian_type_c_type_get(eolian_type_base_type_get(pt));
                  eina_strbuf_append(params_full_imp, ", Eina_Promise_Owner *");
                  eina_strbuf_append(params_full_imp, prn);
                  if (is_empty && !dfv)
                    eina_strbuf_append(params_full_imp, " EINA_UNUSED");
                  eina_strbuf_append(params, "__eo_promise");
               }
             else
               {
                  eina_strbuf_append(params_full_imp, ", ");
                  eina_strbuf_append(params_full_imp, ptn);
                  if (!had_star)
                    eina_strbuf_append_char(params_full_imp, ' ');
                  eina_strbuf_append(params_full_imp, add_star);
                  eina_strbuf_append(params_full_imp, prn);
                  if (!dfv && is_empty)
                    eina_strbuf_append(params_full_imp, " EINA_UNUSED");
                  eina_strbuf_append(params, prn);
               }

             eina_strbuf_append(params_full, ", ");
             eina_strbuf_append(params_full, ptn);
             if (!had_star)
               eina_strbuf_append_char(params_full, ' ');
             eina_strbuf_append(params_full, add_star);
             eina_strbuf_append(params_full, prn);

             if (is_auto)
               {
                  if (ftype == EOLIAN_PROP_SET)
                    eina_strbuf_append_printf(params_init, "   %s = pd->%s;\n", prn, prn);
                  else
                    {
                       eina_strbuf_append_printf(params_init, "   if (%s) *%s = pd->%s\n",
                                                 prn, prn, prn);
                    }
               }
             else if ((ftype != EOLIAN_PROP_SET) && dfv)
               {
                  Eolian_Value val = eolian_expression_eval(dfv, EOLIAN_MASK_ALL);
                  if (val.type)
                    {
                       Eina_Stringshare *vals = eolian_expression_value_to_literal(&val);
                       eina_strbuf_append_printf(params_init, "   if (%s) *%s = %s;",
                                                 prn, prn, vals);
                       eina_stringshare_del(vals);
                       if (eolian_expression_type_get(dfv) == EOLIAN_EXPR_NAME)
                         {
                            Eina_Stringshare *vs = eolian_expression_serialize(dfv);
                            eina_strbuf_append_printf(params_init, " /* %s */", vs);
                            eina_stringshare_del(vs);
                         }
                       eina_strbuf_append_char(params_init, '\n');
                    }
               }

             eina_stringshare_del(ptn);
          }
        eina_iterator_free(itr);
     }

   Eina_Bool impl_same_class = (eolian_implement_class_get(impl) == cl);
   Eina_Bool impl_need = EINA_TRUE;
   if (impl_same_class && eolian_function_is_virtual_pure(fid, ftype))
     impl_need = EINA_FALSE;

   Eina_Stringshare *rtpn = rtp ? eolian_type_c_type_get(rtp)
                                : eina_stringshare_add("void");

   char *cnamel, *ocnamel = NULL;
   if (!eo_gen_class_names_get(cl, NULL, NULL, &cnamel))
     goto end;
   if (!eo_gen_class_names_get(eolian_implement_class_get(impl), NULL, NULL, &ocnamel))
     goto end;

   if (impl_need)
     {
        /* figure out the data type */
        Eina_Bool is_cf = eolian_function_is_class(fid);
        const char *dt = eolian_class_data_type_get(cl);
        char adt[128];
        if (is_cf || (dt && !strcmp(dt, "null")))
          snprintf(adt, sizeof(adt), "void");
        else if (dt)
          snprintf(adt, sizeof(adt), "%s", dt);
        else
          snprintf(adt, sizeof(adt), "%s_Data", eolian_class_full_name_get(cl));

        eina_strbuf_append_char(buf, '\n');
        /* no need for prototype with empty/auto impl */
        if (!is_empty && !is_auto)
          {
             /* T _class_name[_orig_class]_func_name_suffix */
             eina_strbuf_append(buf, rtpn);
             if (!strchr(rtpn, '*'))
               eina_strbuf_append_char(buf, ' ');
             eina_strbuf_append_char(buf, '_');
             eina_strbuf_append(buf, cnamel);
             if (!impl_same_class)
               eina_strbuf_append_printf(buf, "_%s", ocnamel);
             eina_strbuf_append_char(buf, '_');
             eina_strbuf_append(buf, eolian_function_name_get(fid));
             eina_strbuf_append(buf, func_suffix);
             /* ([const ]Eo *obj, Data_Type *pd, impl_full_params); */
             eina_strbuf_append_char(buf, '(');
             if (eolian_function_object_is_const(fid))
               eina_strbuf_append(buf, "const ");
             eina_strbuf_append(buf, "Eo *obj, ");
             eina_strbuf_append(buf, adt);
             eina_strbuf_append(buf, " *pd");
             eina_strbuf_append(buf, eina_strbuf_string_get(params_full_imp));
             eina_strbuf_append(buf, ");\n\n");
          }

        if (is_empty || is_auto || eina_strbuf_length_get(params_init))
          {
             /* we need to give the internal function name to Eo,
              * use this hash table as indication
              */
             eina_hash_add(_funcs_params_init,
                 eina_stringshare_add(eolian_function_name_get(fid)), (void *)ftype);
             /* generation of intermediate __eolian_... */
             eina_strbuf_append(buf, "static ");
             eina_strbuf_append(buf, rtpn);
             if (!strchr(rtpn, '*'))
               eina_strbuf_append_char(buf, ' ');
             eina_strbuf_append(buf, "__eolian_");
             eina_strbuf_append(buf, cnamel);
             if (!impl_same_class)
               eina_strbuf_append_printf(buf, "_%s", ocnamel);
             eina_strbuf_append_char(buf, '_');
             eina_strbuf_append(buf, eolian_function_name_get(fid));
             eina_strbuf_append(buf, func_suffix);
             eina_strbuf_append_char(buf, '(');
             if (eolian_function_object_is_const(fid))
               eina_strbuf_append(buf, "const ");
             eina_strbuf_append(buf, "Eo *obj");
             if (is_empty || is_auto)
               eina_strbuf_append(buf, " EINA_UNUSED");
             eina_strbuf_append(buf, ", ");
             eina_strbuf_append(buf, adt);
             eina_strbuf_append(buf, " *pd");
             eina_strbuf_append(buf, eina_strbuf_string_get(params_full_imp));
             eina_strbuf_append(buf, ")\n{\n");
          }
        if (eina_strbuf_length_get(params_init))
          eina_strbuf_append(buf, eina_strbuf_string_get(params_init));
        if (is_empty || is_auto)
          {
             if (rtp)
               {
                  const char *vals = NULL;
                  if (def_ret)
                    {
                       Eolian_Value val = eolian_expression_eval(def_ret, EOLIAN_MASK_ALL);
                       if (val.type)
                         vals = eolian_expression_value_to_literal(&val);
                    }
                  eina_strbuf_append_printf(buf, "   return %s;\n", vals ? vals : "0");
                  eina_stringshare_del(vals);
               }
             eina_strbuf_append(buf, "}\n\n");
          }
        else if (eina_strbuf_length_get(params_init))
          {
             eina_strbuf_append(buf, "   ");
             if (rtp)
               eina_strbuf_append(buf, "return ");
             eina_strbuf_append_char(buf, '_');
             eina_strbuf_append(buf, cnamel);
             if (!impl_same_class)
               eina_strbuf_append_printf(buf, "_%s", ocnamel);
             eina_strbuf_append_char(buf, '_');
             eina_strbuf_append(buf, eolian_function_name_get(fid));
             eina_strbuf_append(buf, func_suffix);
             eina_strbuf_append(buf, "(obj, pd, ");
             eina_strbuf_append(buf, eina_strbuf_string_get(params));
             eina_strbuf_append(buf, ");\n}\n\n");
          }
     }

   if (impl_same_class)
     {
        /* XXX: bad */
        if (has_promise)
          {
             eina_strbuf_append_printf(buf,
                                       "#undef _EFL_OBJECT_API_BEFORE_HOOK\n#undef _EFL_OBJECT_API_AFTER_HOOK\n#undef _EFL_OBJECT_API_CALL_HOOK\n"
                                       "#define _EFL_OBJECT_API_BEFORE_HOOK _EINA_PROMISE_BEFORE_HOOK(%s, %s%s)\n"
                                       "#define _EFL_OBJECT_API_AFTER_HOOK _EINA_PROMISE_AFTER_HOOK(%s)\n"
                                       "#define _EFL_OBJECT_API_CALL_HOOK(x) _EINA_PROMISE_CALL_HOOK(EFL_FUNC_CALL(%s))\n\n",
                                       promise_param_type, rtpn,
                                       eina_strbuf_string_get(params_full_imp),
                                       promise_param_name,
                                       eina_strbuf_string_get(params));
          }

        void *data;
        Eina_Iterator *itr = eolian_property_keys_get(fid, ftype);
        Eina_Bool has_params = eina_iterator_next(itr, &data);
        eina_iterator_free(itr);

        if (!has_params && !var_as_ret)
          {
             if (is_prop)
               itr = eolian_property_values_get(fid, ftype);
             else
               itr = eolian_function_parameters_get(fid);
             has_params = eina_iterator_next(itr, &data);
             eina_iterator_free(itr);
          }

        eina_strbuf_append(buf, "EOAPI EFL_");
        if (!strcmp(rtpn, "void"))
          eina_strbuf_append(buf, "VOID_");
        eina_strbuf_append(buf, "FUNC_BODY");
        if (has_params)
          eina_strbuf_append_char(buf, 'V');
        if ((ftype == EOLIAN_PROP_GET) || eolian_function_object_is_const(fid)
            || eolian_function_is_class(fid))
          {
             eina_strbuf_append(buf, "_CONST");
          }
        eina_strbuf_append_char(buf, '(');

        Eina_Stringshare *eofn = eolian_function_full_c_name_get(fid, ftype, EINA_FALSE);
        eina_strbuf_append(buf, eofn);
        eina_stringshare_del(eofn);

        if (strcmp(rtpn, "void"))
          {
             const char *vals = NULL;
             if (def_ret)
               {
                  Eolian_Value val = eolian_expression_eval(def_ret, EOLIAN_MASK_ALL);
                  if (val.type)
                    vals = eolian_expression_value_to_literal(&val);
               }
             eina_strbuf_append_printf(buf, ", %s, %s", rtpn, vals ? vals : "0");
             if (vals && (eolian_expression_type_get(def_ret) == EOLIAN_EXPR_NAME))
               {
                  Eina_Stringshare *valn = eolian_expression_serialize(def_ret);
                  eina_strbuf_append_printf(buf, " /* %s */", valn);
                  eina_stringshare_del(valn);
               }
             eina_stringshare_del(vals);
          }
        if (has_params)
          {
             eina_strbuf_append(buf, ", EFL_FUNC_CALL(");
             eina_strbuf_append(buf, eina_strbuf_string_get(params));
             eina_strbuf_append_char(buf, ')');
             eina_strbuf_append(buf, eina_strbuf_string_get(params_full));
          }

        eina_strbuf_append(buf, ");\n");

        if (has_promise)
          {
             eina_strbuf_append(buf, "\n#undef _EFL_OBJECT_API_BEFORE_HOOK\n#undef _EFL_OBJECT_API_AFTER_HOOK\n#undef _EFL_OBJECT_API_CALL_HOOK\n"
                                     "#define _EFL_OBJECT_API_BEFORE_HOOK\n#define _EFL_OBJECT_API_AFTER_HOOK\n"
                                     "#define _EFL_OBJECT_API_CALL_HOOK(x) x\n");
          }
     }

end:
   free(cnamel);
   free(ocnamel);

   eina_stringshare_del(rtpn);

   eina_stringshare_del(promise_param_name);
   eina_stringshare_del(promise_param_type);

   eina_strbuf_free(params);
   eina_strbuf_free(params_full);
   eina_strbuf_free(params_full_imp);
   eina_strbuf_free(params_init);
}

void
eo_gen_source_gen(const Eolian_Class *cl, Eina_Strbuf *buf)
{
   if (!cl)
     return;

   char *cname = NULL, *cnameu = NULL, *cnamel = NULL;
   if (!eo_gen_class_names_get(cl, &cname, &cnameu, &cnamel))
     return;

   _funcs_params_init = eina_hash_stringshared_new(NULL);

   /* event section, they come first */
   {
      Eina_Iterator *itr = eolian_class_events_get(cl);
      Eolian_Event *ev;
      EINA_ITERATOR_FOREACH(itr, ev)
        {
           Eina_Stringshare *evn = eolian_event_c_name_get(ev);
           eina_strbuf_append(buf, "EOAPI const Efl_Event_Description _");
           eina_strbuf_append(buf, evn);
           eina_strbuf_append(buf, " =\n   EFL_EVENT_DESCRIPTION");
           if (eolian_event_is_hot(ev))
             eina_strbuf_append(buf, "_HOT");
           if (eolian_event_is_restart(ev))
             eina_strbuf_append(buf, "_RESTART");
           eina_strbuf_append_printf(buf, "(\"%s\");\n", eolian_event_name_get(ev));
           eina_stringshare_del(evn);
        }
      eina_iterator_free(itr);
   }

   /* method section */
   {
      Eina_Iterator *itr = eolian_class_implements_get(cl);
      const Eolian_Implement *imp;
      EINA_ITERATOR_FOREACH(itr, imp)
        {
           if (eolian_implement_class_get(imp) != cl)
             continue;
           Eolian_Function_Type ftype = EOLIAN_UNRESOLVED;
           const Eolian_Function *fid = eolian_implement_function_get(imp, &ftype);
           switch (ftype)
             {
              case EOLIAN_PROP_GET:
              case EOLIAN_PROP_SET:
                _gen_func(cl, fid, ftype, buf, imp);
                break;
              case EOLIAN_PROPERTY:
                _gen_func(cl, fid, EOLIAN_PROP_SET, buf, imp);
                _gen_func(cl, fid, EOLIAN_PROP_GET, buf, imp);
                break;
              default:
                _gen_func(cl, fid, EOLIAN_UNRESOLVED, buf, imp);
             }
        }
      eina_iterator_free(itr);
   }

   eina_hash_free(_funcs_params_init);
}
