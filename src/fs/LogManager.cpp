#include "LogManager.h"
#include "Kernel.h"
#include "Video.h"

//Start of the Log Super Block
LogSuperBlock g_logspb;


// BlockDevice.cpp�ж����������ATABlockDevice��ʵ��
// extern ATABlockDevice g_ATADevice;

LogSuperBlock::LogSuperBlock(){}

LogSuperBlock::~LogSuperBlock(){}

void LogSuperBlock:: AddTxB()
{
  this->TxB = LogManager::StartExist;
  Kernel::Instance().GetLogManager().LogSpbUpdate();
  Diagnose::Write("AddTxB\n");
}

void  LogSuperBlock::AddTxE()
{
  this->TxE = LogManager::EndExist;
  Kernel::Instance().GetLogManager().LogSpbUpdate();
  Diagnose::Write("AddTxE\n");
}

void LogSuperBlock::AddCKPT()
{
  this->CKPT = LogManager::CheckPointExist;
  Kernel::Instance().GetLogManager().LogSpbUpdate();
  Diagnose::Write("AddCKPT\n");
}

void LogSuperBlock::InitFlag()
{
  this->TxB = 0;
  this->TxE = 0;
  this->CKPT = 0;
  this->log_pointer = 0;
}

//Start of the LogSystem
LogManager::LogManager()
{
  this->Lspb = NULL;
}
LogManager::~LogManager()
{

}

void LogManager::Initialize()
{
  // this->Lspb = &g_logspb;
  this->m_buffer_manager = &Kernel::Instance().GetBufferManager();
  this->m_device_manager = &Kernel::Instance().GetDeviceManager();
  this->spbbuf.b_wcount = LogManager::BlKSIZE;
  this->spbbuf.b_blkno  = LogManager::LOG_SUPER_BLOCK_SECTOR_NUMBER;
  this->spbbuf.b_dev = LogManager::LOGDEV;
  this->resetreadbuf(); //����readbuf
}
//����־���superblock,���ļ�ϵͳ��SuperBlock��ͬ��
//��־���superblock, ��ʼ����ʱ����Ҫ���룬�����ݱ�־λ�ж�д�������
//Ȼ����ѡ���Եػָ���release
void LogManager::Load_LogSuperBlock()
{
  Diagnose::Write("LogManager::Load_LogSuperBlock()...Start \n");
  this->Lspb = &g_logspb; //new add
  this->spbbuf.b_addr = (unsigned char * )&g_logspb;
  this->spbbuf.b_flags = Buf::B_READ | Buf::B_ISLOG;
  this->m_device_manager->GetBlockDevice(Utility::GetMajor(this->spbbuf.b_dev)).Strategy(&(this->spbbuf));
  this->m_buffer_manager->IOWait(&(this->spbbuf));

}


// Write Meta data after Real Data
void LogManager::LogSpbUpdate()
{

  this->spbbuf.b_flags = Buf::B_WRITE | Buf::B_ISLOG; //Ĭ����д��������д��IODONE����
  this->m_device_manager->GetBlockDevice(Utility::GetMajor(this->spbbuf.b_dev)).Strategy(&(this->spbbuf));
  this->m_buffer_manager->IOWait(&(this->spbbuf));
}




void LogManager::logging()
{
  Buf *log_buf = this->m_buffer_manager->getmbuf();
  this->Lspb->log_pointer = 0;
  for (int i = 0; i < BufferManager::NBUF; ++i)
	{
		//��Ҫд��buf���ȵǼ���log ��
		if((log_buf[i].b_flags & Buf::B_ISLOG) == Buf::B_ISLOG)
		{
      this->pointbuf[this->Lspb->log_pointer++]=&(log_buf[i]);
		}
	}

  for(int i=0; i<this->Lspb->log_pointer;i++)
  {
    this->readbuf[i].b_addr = this->pointbuf[i]->b_addr;
    this->Lspb->LogBufferBlkno[i] = this->pointbuf[i]->b_blkno;        // Ҫд��������ַ
    this->Lspb->LogBufferDev[i] = this->pointbuf[i]->b_dev;        // Ҫд����豸
    this->Lspb->LogBufferWcount[i] = BufferManager::BUFFER_SIZE;  // ��Ҫ������ֽ�����
  }
  
  this->Lspb->AddTxB();
  for(int i=0; i<this->Lspb->log_pointer;i++)
  {
    Diagnose::Write("write log blkno %d \n",this->readbuf[i].b_blkno);
    this->readbuf[i].b_dev   = LogManager::LOGDEV;
    this->readbuf[i].b_blkno = LogManager::LOG_DATA_START+i;
    this->readbuf[i].b_flags = Buf::B_ISLOG;
    this->m_device_manager->GetBlockDevice(Utility::GetMajor(LogManager::LOGDEV)).Strategy(&(this->readbuf[i]));
    this->m_buffer_manager->IOWait(&(this->readbuf[i]));
  } 

  this->Lspb->AddTxE();
  for(int i=0; i<this->Lspb->log_pointer;i++)
  {
    Diagnose::Write("write disk blkno %d \n",this->pointbuf[i]->b_blkno);
    this->pointbuf[i]->b_flags |= Buf::B_ISLOG;  
    this->m_device_manager->GetBlockDevice(Utility::GetMajor(this->pointbuf[i]->b_dev)).Strategy(this->pointbuf[i]);
    this->m_buffer_manager->IOWait(this->pointbuf[i]);
  } 
  this->Lspb->AddCKPT();
  this->ReleaseLog();
  return;
}

// �ӻ���鵽���̣�д��־��
void LogManager::WriteLog(Buf *bp)
{
 	bp->b_flags &= ~(Buf::B_READ | Buf::B_ERROR);
	bp->b_flags |= (Buf::B_ISLOG|Buf::B_DONE | Buf::B_ASYNC);
	this->m_buffer_manager->Brelse(bp);
	return;
}

void LogManager::Recover()
{
  Diagnose::Write("LogManager::Recover()����\n");
  if((this->Lspb->TxB == LogManager::StartExist)
  &&(this->Lspb->TxE == LogManager::EndExist)
  &&(this->Lspb->CKPT !=LogManager::CheckPointExist))
  {
    Diagnose::Write("TxB and TxE,NO CKPT, Recover\n");
    this->RedoLog();
    this->ReleaseLog(); //redo֮�������־λ
  }
  else if((this->Lspb->TxB == LogManager::StartExist)
  &&(this->Lspb->TxE != LogManager::EndExist)
  &&(this->Lspb->CKPT !=LogManager::CheckPointExist))
  {
    Diagnose::Write("Only TxB, Abandon Log\n");
    this->ReleaseLog();
  }
  else
  {
    Diagnose::Write("No need to recover\n");
    this->ReleaseLog();
  }
}

void LogManager::RedoLog()
{
  unsigned char * addr_buf = this->m_buffer_manager->getBuffer();
  for (int i = 0; i < BufferManager::NBUF; ++i)
	{
      this->readbuf[i].b_addr = addr_buf + BufferManager::BUFFER_SIZE * i;
	}
  this->WriteLog2Disk();
}

void LogManager::ReleaseLog()
{
  this->Lspb->InitFlag();
  this->LogSpbUpdate();
}

//��Redoǰ����Ҫ�������ã�������b_blkno
void LogManager::resetreadbuf()
{
  for(int i=0;i<LogManager::BlKNUM;i++)
  {
    this->readbuf[i].b_blkno = LogManager::LOG_DATA_START+i;
    this->readbuf[i].b_dev = LogManager::LOGDEV;
    this->readbuf[i].b_wcount = LogManager::BlKSIZE;
  }
}



void LogManager:: WriteLog2Disk()//�Ӵ��̵���־�����������������ж�������ָ���±�
{

  // ����־�����е�����ת�Ƶ����̵���������,��ʱ��ָ�뷶ΧӦ��Ϊ<��,logpointerʵ��������־����
  //��
  for (int i_blk = 0; i_blk < this->Lspb->log_pointer ; ++i_blk)
  {    
    this->readbuf[i_blk].b_blkno = this->LOG_DATA_START+i_blk;
    int blknum = this->readbuf[i_blk].b_blkno;
    this->readbuf[i_blk].b_dev = LogManager::LOGDEV;
    this->readbuf[i_blk].b_flags = Buf::B_READ | Buf::B_ISLOG;
    this->m_device_manager->GetBlockDevice(Utility::GetMajor(LogManager::LOGDEV)).Strategy(&(this->readbuf[i_blk]));
    this->m_buffer_manager->IOWait(&(this->readbuf[i_blk]));   
  }
  Diagnose::Write("Read Log to Buffer\n");
  //д
  for(int i =0;i<this->Lspb->log_pointer;i++)
  {
    this->readbuf[i].b_blkno = this->Lspb->LogBufferBlkno[i];
    this->readbuf[i].b_dev = this->Lspb->LogBufferDev[i];
    this->readbuf[i].b_wcount = this->Lspb->LogBufferWcount[i];
    this->readbuf[i].b_flags = Buf::B_ISLOG;
    this->m_device_manager->GetBlockDevice(Utility::GetMajor(this->readbuf[i].b_dev)).Strategy(&(this->readbuf[i]));
    this->m_buffer_manager->IOWait(&(this->readbuf[i]));   
  }
  Diagnose::Write("Write Buffer to Disk\n");
  return;
}

void LogManager::IODone(Buf* bp)
{
  	/*�����жϸÿ��ǲ���log��*/
	if(bp->b_flags & Buf::B_ISLOG)
	{
		bp->b_flags &= ~(Buf::B_ISLOG);
    bp->b_flags |= Buf::B_DONE;
  }
  /* ����I/O��ɱ�־ */
	if(bp->b_blkno>=this->LOG_DATA_START && bp->b_blkno<=LOG_DATA_END)
	{
    bp->b_flags |= Buf::B_DONE;
    bp->b_flags &= (~Buf::B_WANTED);
    Kernel::Instance().GetProcessManager().WakeUpAll((unsigned long)bp);;
	}
	else 
	{
		this->m_buffer_manager->IODone(bp);
	}
	return;
}
