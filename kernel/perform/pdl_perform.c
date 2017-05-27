/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2015 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: Jiexin Wang   <vikindev@outlook.com>                         |
  +----------------------------------------------------------------------+
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include <dlfcn.h>
#include <ffi.h>
#include "../resource/pdl_resource.h"
#include "../../php_pdl.h"
#include "pdl_perform.h"

extern le_pdl;

zend_resource * __library_open(zend_string *path, zend_string *handle_name) {
    PDL_RES *resource;

    resource = (PDL_RES *)emalloc(sizeof(PDL_RES)+1);

    resource->path         = zend_string_copy(path);
    resource->library_name = zend_string_copy(handle_name);
    resource->handle       = dlopen(ZSTR_VAL(resource->path), RTLD_LAZY);

    return zend_register_resource(resource, le_pdl);
}

int __library_call(zval *res, char *function_name, long return_type, zval *param) {

    PDL_RES *resource;

    void *func_handle, *return_val;

    int int_val[10];
    zend_ulong index;
    zval *tmp_val;

    if((resource = (PDL_RES *)zend_fetch_resource(Z_RES_P(res), "pdl", le_pdl)) == NULL) {
        return FAILURE;
    }

    func_handle = dlsym(resource->handle, function_name);

    if(!func_handle) {
        php_error_docref(NULL, E_ERROR, "PDL warning: Unable to get the dynamic link library %s method\n", function_name);
        return FAILURE;
    }

    switch (return_type) {
        case PDL_RETURN_INT:
            return_val  = (long *)return_val;
            func_handle = (long *)func_handle;
            break;
        case PDL_RETURN_DOUBLE:
            return_val  = (double *)return_val;
            func_handle = (double *)func_handle;
            break;
        case PDL_RETURN_CHAR:
            return_val  = (char *)return_val;
            func_handle = (char *)func_handle;
            break;
    }

    ZEND_HASH_FOREACH_NUM_KEY_VAL(Z_ARRVAL_P(param), index, tmp_val){
                switch (Z_TYPE_P(tmp_val)) {
                    case IS_STRING :
                        break;
                    case IS_LONG :
                        int_val[index] = Z_LVAL_P(tmp_val);
                        break;
                    case IS_DOUBLE :
                        break;
                }
            }ZEND_HASH_FOREACH_END();

    ffi_cif cif;
    ffi_type *args[1];
    void *values[1];
    char *s;
    int rc;

    /* Initialize the argument info vectors */
    args[0] = &ffi_type_pointer;
    values[0] = &s;

    /* Initialize the cif
     *
     * ffi_prep_cif(ffi_cif *cif, ffi_abi abi, 参数, 返回值类型, 参数类型数组)
     * */
    if (ffi_prep_cif(&cif, FFI_DEFAULT_ABI, 1, &ffi_type_uint, args) == FFI_OK)
    {
        s = "Hello World!";
        ffi_call(&cif, puts, &rc, values);
        /* rc now holds the result of the call to puts */

        /* values holds a pointer to the function's arg, so to
           call puts() again all we need to do is change the
           value of s */
        s = "This is cool!";
        ffi_call(&cif, puts, &rc, values);
    }

//    __asm__ __volatile__ (
//            "mov %2, %%rdi;\n"
//            "mov %3, %%rsi;\n"
//            "call *%1;\n"
//            "mov %%rax, %0;\n"
//            :"=m"(return_val)
//            :"m"(func_handle), "m"(int_val[0]), "m"(int_val[1])
//    );

    php_printf("结果：%d\n", return_val);
    return SUCCESS;
}

//int __library_call(char *path, char *handle_name, char *function_name, long return_type, zval *param) {
//    void *lib_handle, *func_handle, *return_val;
//    int int_val[10];
//    zval *tmp_val;
//    zend_ulong index;
//
//    char *hash_key = emalloc(strlen(handle_name)-2);
//    memset(hash_key, '\0', strlen(handle_name)-2);
//    memcpy(hash_key, handle_name, strlen(handle_name)-3);
//
//    lib_handle = dlopen(path, RTLD_LAZY);
//
//    if(!lib_handle) {
//        php_error_docref(NULL, E_ERROR, "PDL warning: Unable to load dynamic library '%s' - %s\n", path, dlerror());
//        dlerror();
//        return FAILURE;
//    }
//
//    func_handle = dlsym(lib_handle, function_name);
//
//    if(!func_handle) {
//        php_error_docref(NULL, E_ERROR, "PDL warning: Unable to get the dynamic link library %s method\n", function_name);
//        dlclose(lib_handle);
//        return FAILURE;
//    }
//
//    switch (return_type) {
//        case PDL_RETURN_INT:
//            return_val  = (long *)return_val;
//            func_handle = (long *)func_handle;
//            break;
//        case PDL_RETURN_DOUBLE:
//            return_val  = (double *)return_val;
//            func_handle = (double *)func_handle;
//            break;
//        case PDL_RETURN_CHAR:
//            return_val  = (char *)return_val;
//            func_handle = (char *)func_handle;
//            break;
//    }
//
//    ZEND_HASH_FOREACH_NUM_KEY_VAL(Z_ARRVAL_P(param), index, tmp_val){
//                switch (Z_TYPE_P(tmp_val)) {
//                    case IS_STRING :
//                        break;
//                    case IS_LONG :
//                        int_val[index] = Z_LVAL_P(tmp_val);
//                        break;
//                    case IS_DOUBLE :
//                        break;
//                }
//            }ZEND_HASH_FOREACH_END();
//
//    __asm__ __volatile__ (
//            "mov %2, %%rdi;\n"
//            "mov %3, %%rsi;\n"
//            "call *%1;\n"
//            "mov %%rax, %0;\n"
//            :"=m"(return_val)
//            :"m"(func_handle), "m"(int_val[0]), "m"(int_val[1])
//    );
//
//    php_printf("结果：%d\n", return_val);
//
//    dlclose(lib_handle);
//    efree(hash_key);
//
//    return SUCCESS;
//}

