
#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <tee_api_defines.h>
#include <user_ta_header_defines.h>
#include <time.h>
#include "ree_agent_ta.h"
#include "sm3.h"
#define TEE_MAX_MEMORY_SIZE 4 * 1024 * 1024 // TEE创建最大内存空间为5MB
#define HASH_SIZE 32
#define MAX_REFERENCES 16000

// 结构体的内存在后面会释放
TEEReferenceValue *ref;
KernelMessage *mem_data;

static TEE_Result tee_reference_add(void *value, uint32_t value_length);
static TEE_Result tee_reference_match(void *value, uint32_t value_length);
static TEE_Result tee_reference_remove(void *value, uint32_t value_length);
static TEE_Result tee_reference_judge(void *value, uint32_t value_length);
static TEE_Result tee_reference_sm3(void *value, uint32_t value_length);
void *teebuf;       // 临时缓冲区，用来传递REE侧的数据
uint32_t *sm3_hash; // 用来测试，存储结构体中的摘要值
const char object_id[] = "YHT";
const uint32_t object_size = sizeof(object_id);
const uint32_t flags = TEE_DATA_FLAG_ACCESS_READ | TEE_DATA_FLAG_ACCESS_WRITE_META | TEE_DATA_FLAG_ACCESS_WRITE | TEE_DATA_FLAG_SHARE_READ | TEE_DATA_FLAG_SHARE_WRITE;

// 打印函数
void tee_printf(void *buffer, uint32_t size)
{
    for (int i = 0; i < size / 4; i++)
    {
        printf("%08x", ((uint32_t *)buffer)[i]);
    }
    printf("\n");
}

// 将REE传递来的基准值传入基准值结构体中，此函数的调用在TA_InvokeCommandEntryPoint中实现
static TEE_Result ref_temporary_storage_buffer(uint32_t param_types, TEE_Param params[4])
{
    // DMSG("Debug YHT : Allocated shared_memory in TA at address %p with size %u bytes\n", params[0].memref.buffer, params[0].memref.size);
    teebuf = TEE_Malloc(params[0].memref.size, TEE_MALLOC_FILL_ZERO);
    sm3_hash = TEE_Malloc(HASH_SIZE, TEE_MALLOC_FILL_ZERO);
    memcpy(teebuf, params[0].memref.buffer, params[0].memref.size);
    //----------------------------测试用的代码-----------------------------------------------------------------------------------------------------
    // 480/32 = 15 ,为了测试正常情况，此处选择使用第16个基准值作为后续的增、删、查、度量的测试用例
    unsigned char *digest_ptr = (unsigned char *)teebuf + 480;
    memcpy(sm3_hash, digest_ptr, 32);

    tee_printf(sm3_hash, HASH_SIZE);
    // 测试REE应用层异常情况
    /* unsigned char a = '1';
    unsigned char *digest_ptr = &a;
    int ret = calculate_sm3(digest_ptr, sizeof(uint32_t), sm3_hash);
    tee_printf(sm3_hash, HASH_SIZE); */

    ref = (TEEReferenceValue *)TEE_Malloc(sizeof(TEEReferenceValue), TEE_MALLOC_FILL_ZERO);
    if (ref == NULL)
    {
        printf("debuf yht : Failed to allocate ref memory");
        return TEE_ERROR_OUT_OF_MEMORY;
    }

    ref->value = teebuf;
    ref->length = params[0].memref.size;

    // 测试代码，配置初始化TEE_Message参数
    printf("00000000000000000000000000000000000000000000000000000000000000000000000000000000\n");
    mem_data = (KernelMessage *)TEE_Malloc(sizeof(KernelMessage), TEE_MALLOC_FILL_ZERO);
    mem_data->operator_type = TA_REE_AGENT_CMD_MATCH_REFERENCE;
    mem_data->baseline_type = 0;
    mem_data->num = 1;
    mem_data->result = -1;
    DMSG("************************************************************************");
    tee_printf(sm3_hash, HASH_SIZE);
    // mem_data->SM3_array = (Baseline *)TEE_Malloc(mem_data->num * HASH_SIZE, TEE_MALLOC_FILL_ZERO);
    mem_data->SM3_array[0] = *(Baseline *)sm3_hash;
    tee_printf(mem_data->SM3_array, mem_data->num * HASH_SIZE);
    printf("11111111111111111111111111111111111111111111111111111111111111111111111111111111\n");

    return TEE_SUCCESS;
}

// 对于内核端的请求做出回应
static TEE_Result kernel_temporary_storage_buffer(uint32_t param_types, TEE_Param params[4])
{
    TEE_Result res;
    // mem_data = (KernelMessage *)(params[0].memref.buffer);
    // DMSG("size of TEE Message: %d", sizeof(mem_data));
    switch (mem_data->operator_type)
    {
    case TA_REE_AGENT_CMD_SM3:
        // 报文里的数据计算哈希传进持久化内存
        res = tee_reference_sm3(mem_data->SM3_array, mem_data->num);
        return res;
    case TA_REE_AGENT_CMD_ADD_REFERENCE:
        // 报文里的全部的基准值增加进持久化内存
        res = tee_reference_add(mem_data->SM3_array, mem_data->num * HASH_SIZE);
        break;
    case TA_REE_AGENT_CMD_DEL_REFERENCE:
        // 调用删除基准值的函数
        res = tee_reference_remove(mem_data->SM3_array, mem_data->num * HASH_SIZE);
        break;
    case TA_REE_AGENT_CMD_MATCH_REFERENCE:
        // 调用查看基准值的函数
        res = tee_reference_match(mem_data->SM3_array, mem_data->num * HASH_SIZE);
        break;
    case TA_REE_AGENT_CMD_JUDGE_REFERENCE:
        // 调用度量基准值的函数
        res = tee_reference_judge(mem_data->SM3_array, mem_data->num * HASH_SIZE);
        break;
    default:
        res = TEE_ERROR_BAD_PARAMETERS;
        break;
    }
    mem_data->result = res;
    return res;
}

// 创建持久化内存(为了测试以下功能，暂且存入SM3摘要值)
static TEE_Result create_persistent_mem(void)
{
    TEE_Result res;
    TEE_ObjectHandle object = TEE_HANDLE_NULL;
    TEE_ObjectInfo object_info;
    size_t count;
    /* TEE_CloseObject(object);
    TEE_CloseAndDeletePersistentObject(object); */

    DMSG("Debug YHT: create_persistent_mem has been called");

    // 创建持久化内存***********************************
    // DMSG("Debug YHT: TEE_CreatePersistentObject has been called");
    /* // 测试创建持久化内存
    const void *p = TEE_Malloc(HASH_SIZE * MAX_REFERENCES * HASH_SIZE, TEE_MALLOC_FILL_ZERO); */
    res = TEE_CreatePersistentObject(TEE_STORAGE_PRIVATE, (void *)object_id, object_size, flags, TEE_HANDLE_NULL, NULL, 0, &object);
    if (res != TEE_SUCCESS)
    {
        DMSG("TEST: TEE_CreatePersistentObject failed 0x%08x", res);
        DMSG("持久化内存无法创建！");
        return res;
    }
    // DMSG("Debug YHT: TEE_CreatePersistentObject is success");
    TEE_GetObjectInfo(object, &object_info);
    printf("TEST:持久化内存刚创建时的大小: %d\n", object_info.dataSize);
    // 最初始基准值准备写入, 写入数据***********************************
    DMSG("Debug YHT: TEE_WriteObjectData has been called");
    res = TEE_WriteObjectData(object, ref->value, ref->length);
    /* //测试边界异常
    void *mem_test = TEE_Malloc(HASH_SIZE * MAX_REFERENCES, TEE_MALLOC_FILL_ZERO);
    res = TEE_WriteObjectData(object , mem_test , HASH_SIZE*MAX_REFERENCES); */
    if (res != TEE_SUCCESS)
    {
        DMSG("TEE_WriteObjectData failed 0x%08x", res);
        TEE_CloseObject(object);
        return res;
    }

    TEE_GetObjectInfo(object, &object_info);
    //tee_printf(ref->value, ref->length);
    printf("TEST:持久化内存创建后写入最初基准值后的大小: %d\n", object_info.dataSize);
    TEE_CloseObject(object);

    /*     // 读取数据
        res = read_persistent_mem(object2);
        if (res != TEE_SUCCESS)
        {
            DMSG("Debug YHT: read_persistent_mem in create failed 0x%08x", res);
            return res;
        }
        TEE_CloseObject(object2); */

    printf("#####################%d\n", mem_data->num);

    return TEE_SUCCESS;
}

// 增加内存数据库中的基准值, 原函数为int型，此处为了调试方便暂时改为TEE_RESULT型，后续待确定
static TEE_Result tee_reference_add(void *value, uint32_t value_length)
{
    TEE_Result res;
    TEE_ObjectInfo obj_info;
    TEE_ObjectHandle obj_add = TEE_HANDLE_NULL;
    // TEE_ObjectHandle obj_add2 = TEE_HANDLE_NULL;
    uint32_t cur_pos;

    DMSG("Debug YHT: 增加基准值 in add has been called");
    // 打开持久化内存
    res = TEE_OpenPersistentObject(TEE_STORAGE_PRIVATE, (void *)object_id, object_size, flags, &obj_add);
    if (res != TEE_SUCCESS)
    {
        DMSG("TEE_OpenPersistentObject failed 0x%08x", res);
        TEE_CloseObject(obj_add);
        return res;
    }

    // 定位指针，必须要定位才能写入
    TEE_GetObjectInfo(obj_add, &obj_info);
    if (obj_info.dataSize / 32 >= MAX_REFERENCES)
    {
        DMSG("持久化内存中的数据存储达到极限");
        return TEE_ERROR_STORAGE_NO_SPACE;
    }
    // printf("TEST:准备对持久化内存进行写入，此时持久化内存的大小为: %d\n", obj_info.dataSize);
    cur_pos = obj_info.dataSize;
    res = TEE_SeekObjectData(obj_add, cur_pos, TEE_DATA_SEEK_SET);
    if (res != TEE_SUCCESS)
    {
        printf("Fail: pri_obj_data(seek ending)\n");
        TEE_CloseObject(obj_add);
        return res;
    }
    // 写入数据，并且最后关闭句柄，保存数据
    res = TEE_WriteObjectData(obj_add, value, value_length);
    if (res != TEE_SUCCESS)
    {
        printf("TEE_WriteObjectData failed 0x%08x \n", res);
        TEE_CloseObject(obj_add);
        return res;
    }
    TEE_GetObjectInfo(obj_add, &obj_info);
    tee_printf(value, value_length);
    printf("TEST:调用add接口写入数据至持久化内存后的大小: %d\n", obj_info.dataSize);

    TEE_CloseObject(obj_add);
    DMSG("Debug YHT: 增加基准值 is successed");
    return TEE_SUCCESS;
};

// 匹配内存数据库中的基准值(value和value_length需要传入TEEReferenceValue结构体中的两个数据，这两个数据在ref_temporary_storage_buffer中被初始化)
static TEE_Result tee_reference_match(void *value, uint32_t value_length)
{
    TEE_Result res;
    TEE_ObjectHandle obj_match = TEE_HANDLE_NULL;
    TEE_ObjectInfo obj_info;
    int i; // 循环使用，当匹配到要查询的基准值时，那么基准值的位置时内存数据库中第i+1个

    DMSG("Debug YHT: 匹配基准值 has been called");
    // 打开持久化内存
    res = TEE_OpenPersistentObject(TEE_STORAGE_PRIVATE, (void *)object_id, object_size, flags, &obj_match);
    if (res != TEE_SUCCESS)
    {
        DMSG("TEE_OpenPersistentObject failed 0x%08x", res);
        goto err;
    }
    // DMSG("Debug YHT: TEE_OpenPersistentObject in match is success");

    // 获取持久化对象信息，包括数据总大小和当前位置
    res = TEE_GetObjectInfo1(obj_match, &obj_info);
    if (res != TEE_SUCCESS)
    {
        DMSG("Error getting object info: %x\n", res);
        goto err;
    }
    printf("TEST : LENGTH = %d \n", mem_data->num);
    for (int num = 0; num < mem_data->num; num++)
    {

        printf("TEST : LENGTH = %d , NUM = %d\n", mem_data->num, num);
        for (i = 0; i < obj_info.dataSize / 32; i++)
        {
            uint8_t digest[32];
            uint32_t bytes_read = 0;

            // 定位到摘要值位置
            res = TEE_SeekObjectData(obj_match, i * 32, TEE_DATA_SEEK_SET);
            if (res != TEE_SUCCESS)
            {
                DMSG("Error seeking obj_match data: %x\n", res);
                goto err;
            }

            // 读取摘要值
            res = TEE_ReadObjectData(obj_match, digest, sizeof(digest), &bytes_read);
            if (res != TEE_SUCCESS || bytes_read != sizeof(digest))
            {
                DMSG("Error reading obj_match data: %x\n", res);
                goto err;
            }

            // 比较读取到的摘要值和需要查找的摘要值
            if (memcmp(digest, &(mem_data->SM3_array[num]), sizeof(digest)) == 0)
            {
                // 找到匹配的摘要值，打印并退出循环
                // DMSG("Debug YHT : Found matching digest at position %d\n", i + 1);
                // 打印比较数据，测试用
                // tee_printf(digest, bytes_read);
                break;
            }
        }
        if (i >= obj_info.dataSize / 32)
        {
            DMSG("Debug YHT: 匹配查找函数失败");
            return TEE_ERROR_NO_DATA;
        }
    }
    TEE_CloseObject(obj_match);
    printf("TEST:调用match接口, 查询摘要值位置存储于持久化内存基准库第%d个基准值\n", i + 1);
    DMSG("Debug YHT: 匹配查找函数 is successed");
    return TEE_SUCCESS;

err:
    TEE_CloseObject(obj_match);
    return res;
}

// 删除内存数据库中的基准值
static TEE_Result tee_reference_remove(void *value, uint32_t value_length)
{
    TEE_Result res;
    TEE_ObjectInfo obj_info;
    TEE_ObjectHandle obj_rm = TEE_HANDLE_NULL;
    TEE_ObjectHandle obj_rm2 = TEE_HANDLE_NULL; // 测试用代码
    uint32_t cur_pos;
    void *rm_mem;         // 临时缓冲区，保存被截断数据后面的所有数据
    int i;                // 循环查找使用
    bool is_find = false; // 判断是否找到此基准
    uint32_t bytes_read;
    DMSG("删除基准值 has been called");
    // 打开持久化内存
    res = TEE_OpenPersistentObject(TEE_STORAGE_PRIVATE, (void *)object_id, object_size, flags, &obj_rm);
    if (res != TEE_SUCCESS)
    {
        DMSG("TEE_OpenPersistentObject failed 0x%08x", res);
        goto err;
    }
    // DMSG("Debug YHT: TEE_OpenPersistentObject in match is success");

    // 获取持久化对象信息，包括数据总大小和当前位置
    TEE_GetObjectInfo(obj_rm, &obj_info);
    printf("删除摘要值操作前的持久化内存的大小：%d\n", obj_info.dataSize);

    for (int num = 0; num < mem_data->num; num++)
    {
        for (i = 0; i < obj_info.dataSize / HASH_SIZE; i++)
        {
            uint8_t digest[32]; // 暂存基准值
            bytes_read = 0;

            // 定位到摘要值位置
            res = TEE_SeekObjectData(obj_rm, i * HASH_SIZE, TEE_DATA_SEEK_SET);
            if (res != TEE_SUCCESS)
            {
                DMSG("Error seeking obj_match data: %x\n", res);
                goto err;
            }

            // 读取摘要值
            res = TEE_ReadObjectData(obj_rm, digest, sizeof(digest), &bytes_read);
            if (res != TEE_SUCCESS || bytes_read != sizeof(digest))
            {
                DMSG("Error reading obj_match data: %x\n", res);
                goto err;
            }

            // 比较读取到的摘要值和需要查找的摘要值
            if (memcmp(digest, &(mem_data->SM3_array[num]), sizeof(digest)) == 0)
            {
                // 找到匹配的摘要值，打印并退出循环
                printf("TEST : 发现待删除的基准值位置在基准库的第 %d 个基准值\n", i + 1);
                is_find = true;
                // 打印比较数据，测试用
                // tee_printf(digest, bytes_read);
                // 并且将定位指针指向此处，此处是查找到的基准值的起始处，删除基准值特有
                cur_pos = i * HASH_SIZE;
                break;
            }
        }
        // 判断是否找到此摘要值
        if (!is_find)
        {
            DMSG("Debug YHT: 删除基准值失败！");
            return TEE_ERROR_NO_DATA;
        }

        // 更新obj的数据位置,先保存待删除摘要值的后续部分
        // 读取不需被截断的数据流
        int size = obj_info.dataSize - cur_pos - HASH_SIZE; // 被截断但待写回的数据的大小
        rm_mem = TEE_Malloc(size, TEE_MALLOC_FILL_ZERO);
        bytes_read = 0;
        res = TEE_SeekObjectData(obj_rm, cur_pos + HASH_SIZE, TEE_DATA_SEEK_SET);
        // DMSG("成功定位,准备写入");
        res = TEE_ReadObjectData(obj_rm, rm_mem, size, &bytes_read);
        if (res != TEE_SUCCESS || bytes_read != size)
        {
            DMSG("Debug YHT: Error reading obj_rm data: %x\n", res);
            goto err;
        }
        printf("TEST : 基准值未被删除时持久化内存的大小: %d \n", obj_info.dataSize);

        // 将持久化内存截断到查找到基准值的位置
        // 该函数改变一个数据流的size。如果参数size小于当前数据流的size，则将删除所有超出size的字节；如果参数size大于当前数据流的size，则在数据流末端添加零字节来扩展数据流。
        res = TEE_TruncateObjectData(obj_rm, cur_pos);
        if (res != TEE_SUCCESS)
        {
            DMSG("Error seeking obj_match data: %x\n", res);
            goto err;
        }
        TEE_GetObjectInfo(obj_rm, &obj_info);
        // DMSG("被截断后的持久化内存的大小为%u", obj_info.dataSize);
        //  先更新定位，再写入数据，并且最后关闭句柄，保存数据
        res = TEE_SeekObjectData(obj_rm, obj_info.dataSize, TEE_DATA_SEEK_SET);
        res = TEE_WriteObjectData(obj_rm, rm_mem, size);
        if (res != TEE_SUCCESS)
        {
            printf("TEE_WriteObjectData failed 0x%08x \n", res);
            TEE_CloseObject(obj_rm);
            return res;
        }
        TEE_GetObjectInfo(obj_rm, &obj_info);
        printf("TEST : 单个基准值被删除后持久化内存的大小: %d\n ", obj_info.dataSize);
        DMSG("Debug YHT: 删除基准值 is successed");
    }
    TEE_GetObjectInfo(obj_rm, &obj_info);
    printf("TEST : 所有基准值被删除后持久化内存的大小: %d\n ", obj_info.dataSize);
    TEE_CloseObject(obj_rm);

    //----------------测试用代码-----------------------
    /* res = TEE_OpenPersistentObject(TEE_STORAGE_PRIVATE, (void *)object_id, object_size, flags, &obj_rm2);
    if (res != TEE_SUCCESS)
    {
        DMSG("TEE_OpenPersistentObject failed 0x%08x", res);
        goto err;
    }
    TEE_GetObjectInfo(obj_rm2, &obj_info);
    DMSG("被截断的数据待写入之前持久化内存的大小为%u", obj_info.dataSize);
    TEE_GetObjectInfo(obj_rm2, &obj_info);
    DMSG("不需要被截断的数据重新写入后持久化内存的大小为%u", obj_info.dataSize);
    DMSG("删除持久化内存数据库基准值successed!");
    TEE_CloseObject(obj_rm2); */
    //-------------------------------------------------

    TEE_Free(rm_mem);
    return TEE_SUCCESS;
err:
    TEE_CloseObject(obj_rm);
    return res;
}
// 判定函数，对传输来的数据与基准值进行对比度量
static TEE_Result tee_reference_judge(void *value, uint32_t value_length)
{
    TEE_Result res;
    DMSG("Debug YHT: 度量基准值 has been called");
    res = tee_reference_match(value, value_length);
    if (res != TEE_SUCCESS)
    {
        DMSG("Debug YHT: 度量基准值失败！");
        return res;
    }
    DMSG("Debug YHT: 度量基准值 is successed");
    return TEE_SUCCESS;
}


// 对于一些需要进行SM3计算的，进行计算SM3摘要值
static TEE_Result tee_reference_sm3(void *value, uint32_t value_num)
{
    TEE_Result res;
    uint32_t *sm3 = TEE_Malloc(HASH_SIZE, TEE_MALLOC_FILL_ZERO);
    DMSG("size的值为: %u", value_num);
    for (int i = 0; i < value_num; i++)
    {
        void *digest = value + i * HASH_SIZE;
        int ret = calculate_sm3(digest, HASH_SIZE, sm3);
        if (ret != 0)
        {
            printf("SM3 calculation failed!\n");
            TEE_Free(sm3);
            return TEE_ERROR_SM3;
        }
        DMSG("SM3 SM3 SM3 SM3 SM3 SM3 SM3 SM3 SM3 SM3 SM3 SM3");
        tee_printf(sm3, HASH_SIZE);
        res = tee_reference_add(sm3, HASH_SIZE);
        if (res != TEE_SUCCESS)
        {
            TEE_Free(sm3);
            return res;
        }
    }
    TEE_Free(sm3);
    return TEE_SUCCESS;
}

// 生成度量报告，传递给REE端

TEE_Result TA_CreateEntryPoint(void)
{
    DMSG("has been called");

    return TEE_SUCCESS;
};
void TA_DestroyEntryPoint(void)
{
    DMSG("has been called");
};

TEE_Result TA_OpenSessionEntryPoint(uint32_t param_types, TEE_Param __maybe_unused params[4], void __maybe_unused **sess_ctx)
{
    TEE_Result res;
    uint32_t exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_NONE, TEE_PARAM_TYPE_NONE, TEE_PARAM_TYPE_NONE, TEE_PARAM_TYPE_NONE);
    DMSG("has been called");
    return TEE_SUCCESS;
};
void TA_CloseSessionEntryPoint(void __maybe_unused *sess_ctx)
{
    (void)&sess_ctx; /* Unused parameter */
    TEE_Free(teebuf);
    TEE_Free(mem_data);
    TEE_Free(ref);
    DMSG("Goodbye!\n");
};

TEE_Result TA_InvokeCommandEntryPoint(void __maybe_unused *sess_ctx, uint32_t cmd_id, uint32_t param_types, TEE_Param params[4])
{
    TEE_Result res;
    /* DMSG("TEST: Allocated memory in TA at address %p with size %d bytes\n", params[0].memref.buffer, params[0].memref.size);
    DMSG("TEST: Allocated memory in TA at address %p with size %d bytes\n", params[1].memref.buffer, params[1].memref.size); */

    DMSG("---------------------------------1---------------------------------------");
    if (params[1].value.a == 22 && params[1].value.b == 23)
    {
        DMSG("---------------------------------2---------------------------------------");
        switch (cmd_id)
        {
        case TA_REE_AGENT_CMD_CREATE_PERSISTENT_MEM:
            // 此函数将REE传递来的基准值写入ref结构体中,并且打印出结构体
            res = ref_temporary_storage_buffer(param_types, params);
            if (res != TEE_SUCCESS)
            {
                DMSG("TEE_WriteObjectData failed 0x%08x", res);
                return res;
            }
            return create_persistent_mem();
        case TA_REE_AGENT_CMD_ADD_REFERENCE:
            return tee_reference_add((void *)mem_data->SM3_array, mem_data->num * HASH_SIZE); // 测试用的参数
        case TA_REE_AGENT_CMD_MATCH_REFERENCE:
            // 测试代码
            /*  DMSG("---------------------------------3---------------------------------------");
             res = kernel_temporary_storage_buffer(param_types, params);
             if (res != TEE_SUCCESS)
             {
                 DMSG("Debug YHT: kernel_temporary_storage_buffer failed 0x%08x", res);
                 mem_data->result = -1;
                 return res;
             }
             mem_data->result = 0;
             DMSG("---------------------------------4---------------------------------------"); */
            return tee_reference_match((void *)mem_data->SM3_array, mem_data->num * HASH_SIZE); // 测试用的参数
        case TA_REE_AGENT_CMD_DEL_REFERENCE:
            return tee_reference_remove((void *)mem_data->SM3_array, mem_data->num * HASH_SIZE); // 测试用的参数
        case TA_REE_AGENT_CMD_JUDGE_REFERENCE:
            return tee_reference_judge((void *)mem_data->SM3_array, mem_data->num * HASH_SIZE); // 测试用的参数
        case TA_REE_AGENT_CMD_SM3:
            return tee_reference_sm3((void *)mem_data->SM3_array, mem_data->num );
        default:
            return TEE_ERROR_BAD_PARAMETERS;
        }
    }
    else
    {
        res = kernel_temporary_storage_buffer(param_types, params);
        if (res != TEE_SUCCESS)
        {
            DMSG("Debug YHT: TEE_CreatePersistentObject failed 0x%08x", res);
            mem_data->result = res;
            return res;
        }
        mem_data->result = TEE_SUCCESS;
    }
}