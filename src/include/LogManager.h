#ifndef LOG_MANAGER_H
#define LOG_MANAGER_H

#include "Buf.h"
#include "BUfferManager.h"
// need ATABlockDevice.strategy
#include "BlockDevice.h"

/*The Log form is Whole log */
/* The superblock of log*/
class LogSuperBlock
{
public:
  LogSuperBlock();
  ~LogSuperBlock();
  void AddTxB();
  void AddTxE();
  void AddCKPT();
  void InitFlag(); // clear the three plus one

public:
  int TxB;        /*begin*/
  int TxE;        /*end*/
  int CKPT; //0xFFFFFFFF��ʾ�����˱�ǣ�0x0��ʾû�б��
  // int StartAddr; // The addr of the log sector
  int log_pointer; // point to the stack top 
  int LogBufferBlkno[15];  // Ҫд��������������������  //record the write position of disk -> BitMap
  int LogBufferWcount[15]; //record the Bytes need to transfer
  short LogBufferDev[15]; //record the dev number
  // unsigned char * LogBufferBaddr[15]; //Record the Buf baddr 
  bool padding[512-166];
  // The length is 512 Bytes
};

// LogManager, control the record of system log
class LogManager
{
public:
  static const int StartExist = 0xA;      //    01010
  static const int EndExist = 0x15;       //  10101
  static const int CheckPointExist = 0x9; // 01001

  static const int LOGDEV = 0;
  static const int  BlKNUM = 15;
  static const int  BlKSIZE = 512;
  static const int LOG_SUPER_BLOCK_SECTOR_NUMBER = 184; //start of log sb

  static const int LOG_DATA_Zone_Size = 15; // num of buffer num
  static const int LOG_DATA_START = 185;    // start of data
  static const int LOG_DATA_END = 200 - 1;

public:
  LogSuperBlock *Lspb;

public:
  LogManager();
  ~LogManager();
  void Load_LogSuperBlock();
  void Initialize();
  void LogSpbUpdate();
  void Recover();
  void RedoLog();
  void ReleaseLog();  //clear the superblock area
  // void LogProcessend();
  void WriteLog(Buf *bp); //�ӻ���������־��
  void WriteLog2Disk();  //�Ӵ��̵���־�����������������ж�������ָ���±�
  //void updatebuf(); //����log���
  void logging();
  void IODone(Buf* bp);
  //�������������
  void resetreadbuf();
public:
  // void LogIOWait(Buf* bp); //��Ҫ����
  // void LogIODone(Buf* bp);

private:
  Buf spbbuf; //��д��ʱ�����ܻ���Ҫgetblk��ȡ����ʱ�򣬾������˵ݹ�
  Buf readbuf[BlKNUM]; //Ϊ�˲�ʹ��Bread��������GetBlk,����readbuf��
  Buf * pointbuf[BlKNUM]; 
  unsigned char Rbuffer[BlKSIZE]; //buf.addrָ��Ļ�����
  BufferManager * m_buffer_manager; //��Ҫ�������ģ��(BufferManager)�ṩ�Ľӿ�
  DeviceManager * m_device_manager; //ָ��ȫ��device manager
  // SystemManager
};

#endif
