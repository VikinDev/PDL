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

    void *func_handle, *return_val = NULL;

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

    ffi_cif cif;
    ffi_type *ffi_return_type;

    // 设置参数的返回类型
    switch (return_type) {
        case PDL_RETURN_INT:
            ffi_return_type = &ffi_type_sint;
            break;
        case PDL_RETURN_DOUBLE:
            ffi_return_type = &ffi_type_double;
            break;
        case PDL_RETURN_CHAR:
            ffi_return_type = &ffi_type_uchar;
            break;
    }

    // 参数数量
    unsigned int para_num =  zend_hash_num_elements(Z_ARRVAL_P(param));

    // 根据参数数量 创建 参数数组
    void **values = alloca(sizeof(void *) * para_num);

    // 根据参数数量 创建 参数类型数组
    ffi_type **para_types = alloca(sizeof(ffi_type *) * para_num);

    // 组织参数的数组和参数类型数组
    ZEND_HASH_FOREACH_NUM_KEY_VAL(Z_ARRVAL_P(param), index, tmp_val){
                switch (Z_TYPE_P(tmp_val)) {
                    case IS_STRING :
                        para_types[index] = &ffi_type_uchar;
                        break;
                    case IS_LONG :
                        // 参数类型数组
                        para_types[index] = &ffi_type_sint;

                        // 参数数组
                        void *valuesPtr = alloca(para_types[index]->size);
                        int *argPtr = valuesPtr;
                        *argPtr = Z_LVAL_P(tmp_val);
                        values[index] = valuesPtr;

                        break;
                    case IS_DOUBLE :
                        para_types[index] = &ffi_type_double;
                        break;
                }
            }ZEND_HASH_FOREACH_END();

    // ffi_prep_cif(ffi_cif *cif, ffi_abi abi, 参数数量, 返回值类型, 参数类型数组)
    if (ffi_prep_cif(&cif, FFI_DEFAULT_ABI, para_num, ffi_return_type, para_types) == FFI_OK)
    {
        // 分配返回值内存
        if (ffi_return_type->size) {
            return_val = alloca(ffi_return_type->size);
        }

        // 调用函数
        ffi_call(&cif, func_handle, return_val, values);
    } else {
        php_error_docref(NULL, E_ERROR, "PDL warning: Failed to call %s method!\n", function_name);
        return FAILURE;
    }

//    __asm__ __volatile__ (
//            "mov %2, %%rdi;\n"
//            "mov %3, %%rsi;\n"
//            "call *%1;\n"
//            "mov %%rax, %0;\n"
//            :"=m"(return_val)
//            :"m"(func_handle), "m"(int_val[0]), "m"(int_val[1])
//    );

    php_printf("结果：%d\n", *(int *)return_val);
    return SUCCESS;
}

