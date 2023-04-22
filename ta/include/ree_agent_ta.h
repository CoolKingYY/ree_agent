#ifndef REE_AGENT_TA_H
#define REE_AGENT_TA_H

#define TA_REE_AGENT_UUID { 0x3b3f9a9d, 0x08d5, 0x4028,\
    { 0xaa, 0xe1, 0x87, 0xcb, 0xed, 0x59, 0xe6, 0xb8 } }



/* The function IDs implemented in this TA */
#define TA_REE_AGENT_CMD_CREATE_PERSISTENT_MEM    1 
#define TA_REE_AGENT_CMD_ADD_REFERENCE     2
#define TA_REE_AGENT_CMD_DEL_REFERENCE     3
#define TA_REE_AGENT_CMD_MATCH_REFERENCE     4
#define TA_REE_AGENT_CMD_JUDGE_REFERENCE     5
#define TA_REE_AGENT_CMD_SM3    6


//data for kernel CA
#define u8 unsigned char
#define u32 unsigned int
#define u64 unsigned long 
#define MAX_SYSCALL_NUM		451
#define MAX_SHM_SIZE     128 *1024
#define MAX_HASH_NUM		(MAX_SHM_SIZE - 4) / 32  

 
#define TEE_DATABASE_ELF        0x1 
#define TEE_DATABASE_SO         0x2 
#define TEE_DATABASE_KO         0x4 
#define TEE_DATABASE_MODULE_TEXT        0x8 
#define TEE_DATABASE_KERNEL_TEXT        0x10 
#define TEE_DATABASE_TASK_TEXT          0x20 
#define TEE_DATABASE_MEM_CRITICAL_DATA          0x40 

/* API Error Codes */
#define TEE_SUCCESS                       0x00000000  // 成功
// 从0xF0100001开始是与TEE相对应的错误码
#define TEE_ERROR_CORRUPT_OBJECT          0xF0100001  // 对象已损坏
#define TEE_ERROR_CORRUPT_OBJECT_2        0xF0100002
#define TEE_ERROR_STORAGE_NOT_AVAILABLE   0xF0100003  // 存储不可用
#define TEE_ERROR_STORAGE_NOT_AVAILABLE_2 0xF0100004
#define TEE_ERROR_CIPHERTEXT_INVALID      0xF0100006  // 密文无效
// 从0xFFFF0000开始是通用错误码
#define TEE_ERROR_GENERIC                 0xFFFF0000  // 通用错误
#define TEE_ERROR_ACCESS_DENIED           0xFFFF0001  // 拒绝访问
#define TEE_ERROR_CANCEL                  0xFFFF0002  // 操作被取消
#define TEE_ERROR_ACCESS_CONFLICT         0xFFFF0003  // 访问冲突
#define TEE_ERROR_EXCESS_DATA             0xFFFF0004  // 数据过多
#define TEE_ERROR_BAD_FORMAT              0xFFFF0005  // 格式错误
#define TEE_ERROR_BAD_PARAMETERS          0xFFFF0006  // 无效参数
#define TEE_ERROR_BAD_STATE               0xFFFF0007  // 错误状态
#define TEE_ERROR_ITEM_NOT_FOUND          0xFFFF0008  // 对象未找到
#define TEE_ERROR_NOT_IMPLEMENTED         0xFFFF0009  // 未实现
#define TEE_ERROR_NOT_SUPPORTED           0xFFFF000A  // 不支持
#define TEE_ERROR_NO_DATA                 0xFFFF000B  // 无数据
#define TEE_ERROR_OUT_OF_MEMORY           0xFFFF000C  // 内存不足
#define TEE_ERROR_BUSY                    0xFFFF000D  // 忙
#define TEE_ERROR_COMMUNICATION           0xFFFF000E  // 通信错误
#define TEE_ERROR_SECURITY                0xFFFF000F  // 安全错误
#define TEE_ERROR_SHORT_BUFFER            0xFFFF0010  // 缓冲区过短
#define TEE_ERROR_EXTERNAL_CANCEL         0xFFFF0011  // 外部操作取消
#define TEE_ERROR_SM3                     0xFFFF0012  // 计算SM3出错
// 从0xFFFF300F开始是TEE特定的错误码
#define TEE_ERROR_OVERFLOW                0xFFFF300F  // 溢出
#define TEE_ERROR_TARGET_DEAD             0xFFFF3024  // 目标不可用
#define TEE_ERROR_STORAGE_NO_SPACE        0xFFFF3041  // 存储空间不足
#define TEE_ERROR_SIGNATURE_INVALID       0xFFFF3071  // 签名无效


//struct for use
typedef struct tee_reference_value
{
    unsigned int length; // 摘要值长度
    const void *value;   // 指向摘要值的指针
} TEEReferenceValue;

struct mem_critical_data{
	bool MMU;
	u32 PageRW;
	u32 IDT[32];
	u32 SELinux;
	u64 Syscall_Table[MAX_SYSCALL_NUM];
};

typedef struct SM3_Baseline{   //4+32 
    //u32 filepath; //文件一级相对路径
    u8 Hash[32];
}Baseline;

typedef struct TEE_Message{
    u8 operator_type;   //1
    u8 baseline_type;   //1
    int num; //1 
    int result; //1 备用
    Baseline SM3_array[MAX_HASH_NUM];
}KernelMessage;


#endif /* REE_AGENT_TA_H_ */