/*
    2023.4.7 YHT 将扫描后的whole.log传输至持久化内存"YHT"中，将whole.log的第16个摘要值写入sm3_hash全局变量中，随后使用add函数增加sm3_hash至末尾(没有实际作用，仅仅测试能否写入)，
            随后使用match函数，以sm3_hash为待校验值，与持久化内存中的数据匹配，最后输出匹配成功的摘要值和摘要值序列，测试成功！
    2023.4.10 YHT 测试写入judge接口,测试写入remove接口，成功！
    2023.4.10 YHT 度量报告接口未完成，REE传递的策略是否要写入持久化内存未完成，CA端接口完善未完成
*/

#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <tee_api_defines.h>
#include <user_ta_header_defines.h>
#include "ree_agent_ta.h"
#include "sm3.h"
#define TEE_MAX_MEMORY_SIZE 4 * 1024 * 1024 // TEE创建最大内存空间为5MB
#define MAX_DATA_LEN 500 * 1024
#define HASH_SIZE 32
#define MAX_REFERENCES 16000


// 结构体的内存在后面会释放
TEEReferenceValue *ref;
struct TEE_Message mes;

TEE_Result read_persistent_mem(TEE_ObjectHandle object2);

void *teebuf;                      // 临时缓冲区，用来传递REE侧的数据
unsigned char sm3_hash[HASH_SIZE]; // 用来测试，存储结构体中的摘要值
const char object_id[] = "YHT";
const uint32_t object_size = sizeof(object_id);
const uint32_t flags = TEE_DATA_FLAG_ACCESS_READ | TEE_DATA_FLAG_ACCESS_WRITE_META | TEE_DATA_FLAG_ACCESS_WRITE | TEE_DATA_FLAG_SHARE_READ | TEE_DATA_FLAG_SHARE_WRITE;

// 打印函数,测试函数
void tee_printf(void *buffer, uint32_t size)
{
    for (int i = 0; i < size; i++)
    {
        printf("%02x", ((unsigned char *)buffer)[i]);
    }
    printf("\n");
}


// 对于一些需要进行SM3计算的，进行计算SM3摘要值
static void calculate_sm3_in_ta(void *mem, size_t size)
{
    // 将传递的数据进行SM3计算摘要值
    int ret = calculate_sm3(mem, size, sm3_hash);
    if (ret == 0)
/*     {
        printf("Debug YHT: SM3 Hash: ");
        for (int i = 0; i < HASH_SIZE; i++)
        {
            printf("%02x", sm3_hash[i]);
        }
        printf("\n");
    }
    else */
    {
        printf("SM3 calculation failed!\n");
    }
}

// 将REE传递来的基准值传入基准值结构体中，此函数的调用在TA_InvokeCommandEntryPoint中实现
static TEE_Result ref_temporary_storage_buffer(uint32_t param_types, TEE_Param params[4])
{
    // DMSG("Debug YHT : Allocated shared_memory in TA at address %p with size %u bytes\n", params[0].memref.buffer, params[0].memref.size);
    teebuf = TEE_Malloc(params[0].memref.size, TEE_MALLOC_FILL_ZERO);
    memcpy(teebuf, params[0].memref.buffer, params[0].memref.size);
    tee_printf(teebuf,params[0].memref.size);   //打印出所有的基准值
    //----------------------------测试用的代码-----------------------------------------------------------------------------------------------------
    // 480/32 = 15 ,为了测试，此处选择使用第16个基准值作为后续的增、删、查、度量的测试用例
    unsigned char *digest_ptr = (unsigned char *)teebuf + 480;
    memcpy(sm3_hash, digest_ptr, HASH_SIZE);
    /*     tee_printf(sm3_hash, HASH_SIZE);
        printf("teebuf的大小为: %u\n", params[0].memref.size); */
    ref = (TEEReferenceValue *)TEE_Malloc(sizeof(TEEReferenceValue), TEE_MALLOC_FILL_ZERO);
    if (ref == NULL)
    {
        printf("debuf yht : Failed to allocate ref memory");
        return TEE_ERROR_OUT_OF_MEMORY;
    }

    // 如果传递来的数据需要进行SM3计算，则取消注释下面代码,并且修改上方针对teebuf的代码
    /*     teebuf = TEE_Malloc(HASH_SIZE, TEE_MALLOC_FILL_ZERO);
        calculate_sm3_in_ta(params[0].memref.buffer, params[0].memref.size, teebuf); */

    ref->value = teebuf;
    ref->length = params[0].memref.size;
    return TEE_SUCCESS;
}

// 创建持久化内存(为了测试以下功能，暂且存入SM3摘要值)
static TEE_Result create_persistent_mem(void)
{
    TEE_Result res;
    TEE_ObjectHandle object = TEE_HANDLE_NULL;
    TEE_ObjectHandle object2 = TEE_HANDLE_NULL;
    TEE_ObjectInfo object_info;
    size_t count;
    /* TEE_CloseObject(object);
    TEE_CloseAndDeletePersistentObject(object); */

    DMSG("Debug YHT: create_persistent_mem has been called");

    // 创建持久化内存***********************************
    // DMSG("Debug YHT: TEE_CreatePersistentObject has been called");
    res = TEE_CreatePersistentObject(TEE_STORAGE_PRIVATE, (void *)object_id, object_size, flags, TEE_HANDLE_NULL, NULL, 0, &object);
    if (res != TEE_SUCCESS)
    {
        DMSG("Debug YHT: TEE_CreatePersistentObject failed 0x%08x", res);
        return res;
    }
    // DMSG("Debug YHT: TEE_CreatePersistentObject is success");

    // 最初始基准值准备写入, 写入数据***********************************
    DMSG("Debug YHT: TEE_WriteObjectData has been called");
    res = TEE_WriteObjectData(object, ref->value, ref->length);
    if (res != TEE_SUCCESS)
    {
        DMSG("TEE_WriteObjectData failed 0x%08x", res);
        TEE_CloseObject(object);
        return res;
    }

    TEE_CloseObject(object);

    /*     // 读取数据
        res = read_persistent_mem(object2);
        if (res != TEE_SUCCESS)
        {
            DMSG("Debug YHT: read_persistent_mem in create failed 0x%08x", res);
            return res;
        }
        TEE_CloseObject(object2); */

    return TEE_SUCCESS;
}

// 增加内存数据库中的基准值, 原函数为int型，此处为了调试方便暂时改为TEE_RESULT型，后续待确定
static TEE_Result tee_reference_add(void *value, int value_length)
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
        mes.result = res;
        return res;
    }

    // 定位指针，必须要定位才能写入
    TEE_GetObjectInfo(obj_add, &obj_info);
    if (obj_info.dataSize / 32 >= MAX_REFERENCES)
    {
        DMSG("持久化内存中的数据存储达到极限");
        mes.result = res;
        return TEE_ERROR_STORAGE_NO_SPACE;
    }
    // printf("当前持久化内存的大小为: %d\n", obj_info.dataSize);
    cur_pos = obj_info.dataSize;
    res = TEE_SeekObjectData(obj_add, cur_pos, TEE_DATA_SEEK_SET);
    if (res != TEE_SUCCESS)
    {
        printf("Fail: pri_obj_data(seek ending)\n");
        TEE_CloseObject(obj_add);
        mes.result = res;
        return res;
    }
    // 写入数据，并且最后关闭句柄，保存数据
    res = TEE_WriteObjectData(obj_add, sm3_hash, HASH_SIZE);
    if (res != TEE_SUCCESS)
    {
        printf("TEE_WriteObjectData failed 0x%08x \n", res);
        TEE_CloseObject(obj_add);
        mes.result = res;
        return res;
    }
    TEE_CloseObject(obj_add);

    //-----------------------以下为测试内容-------------------------
    // 读取持久化内存的内容，并且将内容复制给临时缓冲区buffer
    /* res = read_persistent_mem(obj_add2);
    if (res != TEE_SUCCESS)
    {
        DMSG("Debug YHT: read_persistent_mem in create failed 0x%08x", res);
        return res;
    }
    TEE_CloseObject(obj_add2); */
    DMSG("Debug YHT: 增加基准值 is successed");
    return TEE_SUCCESS;
};

// 匹配内存数据库中的基准值(value和value_length需要传入TEEReferenceValue结构体中的两个数据，这两个数据在ref_temporary_storage_buffer中被初始化)
static TEE_Result tee_reference_match(void *value, int value_length)
{
    TEE_Result res;
    TEE_ObjectHandle obj_match = TEE_HANDLE_NULL;
    TEE_ObjectInfo obj_info;
    int i; // 循环使用，当匹配到要查询的基准值时，那么基准值的位置时内存数据库中第i+1个

    if (value_length != HASH_SIZE)
    {
        DMSG("传递的校验数据的大小不正确");
        mes.result = TEE_ERROR_BAD_PARAMETERS;
        return TEE_ERROR_BAD_PARAMETERS;
    }

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
    // 读取持久化对象中的每个32字节的SM3摘要值
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
        if (memcmp(digest, sm3_hash, sizeof(digest)) == 0)
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
        mes.result = TEE_ERROR_NO_DATA;
        return TEE_ERROR_NO_DATA;
    }
    TEE_CloseObject(obj_match); // 为什么此处关闭句柄报错0xFFFF0007
    DMSG("Debug YHT: 匹配查找函数 is successed");
    return TEE_SUCCESS;

err:
    TEE_CloseObject(obj_match);
    mes.result = res;
    return res;
}

// 删除内存数据库中的基准值
int tee_reference_remove(void *value, int value_length)
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
    DMSG("Debug YHT: 删除基准值 has been called");
    // 先匹配基准值
    if (value_length != HASH_SIZE)
    {
        DMSG("传递的校验数据的大小不正确");
        mes.result = TEE_ERROR_BAD_PARAMETERS;
        return TEE_ERROR_BAD_PARAMETERS;
    }
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
    // DMSG("删除摘要值操作前的持久化内存的大小：%u", obj_info.dataSize);

    // 读取持久化对象中的每个32字节的SM3摘要值
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
        if (memcmp(digest, sm3_hash, sizeof(digest)) == 0)
        {
            // 找到匹配的摘要值，打印并退出循环
            // DMSG("Debug YHT : Found matching digest at position %d\n", i + 1);
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
        DMSG("Debug YHT: 匹配查找函数失败");
        mes.result = TEE_ERROR_NO_DATA;
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
    // DMSG("被截断但待写回的数据大小为：%u , 持久化内存的大小为: %u , 写入临时缓冲区的内存数据大小为: %u", size,obj_info.dataSize, bytes_read);

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
        mes.result = res;
        return res;
    }
    DMSG("Debug YHT: 删除基准值 is successed");

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
    mes.result = res;
    return res;
}

// 判定函数，对传输来的数据与基准值进行对比度量
int tee_reference_judge(void *data, int data_length)
{
    TEE_Result res;
    DMSG("Debug YHT: 度量基准值 has been called");
    res = tee_reference_match(data, data_length);
    if (res != TEE_SUCCESS)
    {
        DMSG("Debug YHT: 度量基准值失败！");
        mes.result = -1;
        return -1;
    }
    DMSG("Debug YHT: 度量基准值 is successed");
    mes.result = 0;
    return 0;
}

// 生成度量报告，传递给REE端
// 未完成
int tee_judge_report(){}

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
    uint32_t exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_NONE, TEE_PARAM_TYPE_NONE, TEE_PARAM_TYPE_NONE, TEE_PARAM_TYPE_NONE);

    DMSG("has been called");

    if (param_types != exp_param_types)
        return TEE_ERROR_BAD_PARAMETERS;

    return TEE_SUCCESS;
};
void TA_CloseSessionEntryPoint(void __maybe_unused *sess_ctx)
{
    (void)&sess_ctx; /* Unused parameter */
    TEE_Free(teebuf);
    TEE_Free(ref);
    DMSG("Goodbye!\n");
};

TEE_Result TA_InvokeCommandEntryPoint(void __maybe_unused *sess_ctx, uint32_t cmd_id, uint32_t param_types, TEE_Param params[4])
{
    TEE_Result res;

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
        return create_persistent_mem(); //创建持久化内存并且写入初始基准值
    case TA_REE_AGENT_CMD_ADD_REFERENCE:
        return tee_reference_add(ref->value, ref->length); //测试用的参数
    case TA_REE_AGENT_CMD_MATCH_REFERENCE:
        return tee_reference_match(sm3_hash, HASH_SIZE); // 测试用的参数
    case TA_REE_AGENT_CMD_DEL_REFERENCE:
        return tee_reference_remove(sm3_hash, HASH_SIZE); // 测试用的参数
    case TA_REE_AGENT_CMD_JUDGE_REFERENCE:
        return tee_reference_judge(sm3_hash, HASH_SIZE); // 测试用的参数
    default:
        return TEE_ERROR_BAD_PARAMETERS;
    }
}