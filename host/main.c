#include <err.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <tee_client_api.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <ree_agent_ta.h>
#include "sm3.h"
#define HASH_SIZE 32
UINT32 *sm3_hash;

typedef struct ree_reference_value {
	int length;
	const void* value;
}REEReferenceValue;

//获取存储在REE端的摘要值
void get_reference(REEReferenceValue* ref_struct, char file_name[]) {
	// 读入摘要值文件文件
	FILE* fp = fopen(file_name, "rb");
	if (fp == NULL) {
		printf("DEBUG REE_AGENT : Failed to open SM3_file\n");
		exit(0);
	}
	// 获取文件大小
	fseek(fp, 0L, SEEK_END);
	long int file_size = ftell(fp);
	rewind(fp);
	// 分配一段内存
	unsigned char* buffer = (unsigned char*)malloc(file_size);
	// 检查内存分配是否成功
	if (buffer == NULL) {
		perror("DEBUG REE_AGENT : Failed to allocate memory");
	}
	// 读取文件内容到buffer中
	size_t read_size = fread(buffer, 1, file_size, fp);
	if (read_size != file_size) {
		perror("DEBUG REE_AGENT : Failed to read file");
		exit(EXIT_FAILURE);
	}

	ref_struct->value = malloc(file_size);
	memcpy(ref_struct->value , buffer , file_size);
	ref_struct->length = file_size;

	free(buffer);
	fclose(fp);
}

//请求TEE增加内存数据库中的基准值,未完成
TEEC_Result tee_reference_add(TEEC_Session sesssion, TEEC_Operation operation, void* value, int size, uint32_t err_origin) {
	//TEEC_Result res;
	//调用TA端 写入数据至持久化内存 ， 作为测试，在写入一个摘要值
/* 	printf("debug yht :增加基准值 has been called\n");
	res = TEEC_InvokeCommand(&sess, TA_REE_AGENT_CMD_ADD_REFERENCE, &op, &err_origin);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_InvokeCommand failed with code 0x%x origin 0x%x", res, err_origin);
	printf("debug yht :增加基准值 successed\n"); */
}
//请求TEE删除内存数据库中的基准值
TEEC_Result tee_reference_remove(void* value, int size) {}
//请求TEE查找内存数据库中的基准值
TEEC_Result tee_reference_match(void* value, int size) {}
//向TEE传递开机后的最初基准值
TEEC_Result transfer_reference_to_tee(void* reference, int size) {}


int main(int argc, char* argv[])
{
	TEEC_Result res;
	TEEC_Context ctx;
	TEEC_Session sess;
	TEEC_Operation op;
	TEEC_SharedMemory mem_p;
	TEEC_UUID uuid = TA_REE_AGENT_UUID;
	uint32_t err_origin;
	REEReferenceValue* ref;
	char file[] = "/root/whole.log";

	/* context 初始化 */
	res = TEEC_InitializeContext(NULL, &ctx);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_InitializeContext failed with code 0x%x", res);

	/*打开session*/
	res = TEEC_OpenSession(&ctx, &sess, &uuid, TEEC_LOGIN_PUBLIC, NULL, NULL, &err_origin);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_Opensession failed with code 0x%x origin 0x%x",
			res, err_origin);


	// 动态分配内存来存储 REEReferenceValue 结构体
	ref = (REEReferenceValue*)malloc(sizeof(REEReferenceValue));
	if (ref == NULL) {
		perror("debuf yht : Failed to allocate ref memory");
		exit(EXIT_FAILURE);
	}
	//获取摘要值
	get_reference(ref, file);

	//初始化一系列操作
	memset(&op, 0, sizeof(op));
	memset((void*)&mem_p, 0, sizeof(mem_p));

	printf("DEBUG REE_AGENT : Allocated memory in TA at address %p with size %d bytes\n", ref->value, ref->length);;

	/*设置共享内存的内容和大小*/
	mem_p.buffer = ref->value;
	mem_p.size = ref->length;
	mem_p.flags = TEEC_MEM_INPUT | TEEC_MEM_OUTPUT;

	//注册共享内存
	res = TEEC_RegisterSharedMemory(&ctx, &mem_p);
	if (res != TEEC_SUCCESS) {
		printf("Failed to register DATA shared memory\n");
		TEEC_ReleaseSharedMemory(&mem_p);
		TEEC_CloseSession(&sess);
		exit(0);
	}
	//此处为上锁部分
	TEEC_Value value;
	value.a = 22;
	value.b = 23;

	//初始化操作
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_WHOLE, TEEC_VALUE_INOUT, TEEC_NONE, TEEC_NONE);
	op.params[0].memref.parent = &mem_p;
	op.params[1].value = value;

	//调用TA端 创建持久化内存功能，作为测试，此处已经写入多个摘要值
	printf("debug yht :创建持久化内存 has been called\n");
	res = TEEC_InvokeCommand(&sess, TA_REE_AGENT_CMD_CREATE_PERSISTENT_MEM, &op, &err_origin);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_InvokeCommand failed with code 0x%x origin 0x%x", res, err_origin);
	printf("debug yht :创建持久化内存 successed\n");

	//调用TA端 写入数据至持久化内存 ， 作为测试，在写入一个摘要值
	printf("debug yht :增加基准值 has been called\n");
	res = TEEC_InvokeCommand(&sess, TA_REE_AGENT_CMD_ADD_REFERENCE, &op, &err_origin);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_InvokeCommand failed with code 0x%x origin 0x%x", res, err_origin);
	printf("debug yht :增加基准值 successed\n");

	// 匹配数据
	printf("debug yht :查找基准值 has been called\n");
	res = TEEC_InvokeCommand(&sess, TA_REE_AGENT_CMD_MATCH_REFERENCE, &op, &err_origin); 
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_InvokeCommand failed with code 0x%x origin 0x%x", res, err_origin);
	printf("debug yht :查找基准值 successed\n");

	//删除某个特定的基准值
	printf("debug yht :删除基准值 has been called\n");
	res = TEEC_InvokeCommand(&sess, TA_REE_AGENT_CMD_DEL_REFERENCE, &op, &err_origin);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_InvokeCommand failed with code 0x%x origin 0x%x", res, err_origin);
	printf("debug yht :删除基准值 successed\n");

	//度量某个特定的基准值
	printf("debug yht :度量基准值 has been called\n");
	res = TEEC_InvokeCommand(&sess, TA_REE_AGENT_CMD_JUDGE_REFERENCE, &op, &err_origin);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_InvokeCommand failed with code 0x%x origin 0x%x", res, err_origin);
	printf("debug yht :度量基准值 successed\n");




	//------------------------内核端REE测试代码--------------------------------------------

	//度量某个特定的基准值
	printf("debug yht :计算摘要值 has been called\n");
	res = TEEC_InvokeCommand(&sess, TA_REE_AGENT_CMD_SM3, &op, &err_origin);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_InvokeCommand failed with code 0x%x origin 0x%x", res, err_origin);
	printf("debug yht :计算摘要值 successed\n");


	//关闭文件，释放空间，结束会话
	free((void*)ref->value);
	free(ref); // 然后释放结构体本身的内存
	TEEC_ReleaseSharedMemory(&mem_p);
	TEEC_CloseSession(&sess);
	TEEC_FinalizeContext(&ctx);

	return 0;
}