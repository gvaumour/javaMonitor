#include <fstream>
#include <iostream>
#include <syscall.h>
#include <stdio.h>
#include "pin.H"

using namespace std;


ofstream log_file;
uint64_t i;

bool isVMinit;
bool isGCrunning;
bool isVMstart;

vector<uint64_t> GC_access;
vector<uint64_t> START_access;
vector<uint64_t> INIT_access;
vector<uint64_t> EXEC_access;

enum state
{
	VMSTART,
	VMINIT,
	GCBEGIN,
	GCEND,
	NUM_STATE
};


const char * StripPath(const char * path)
{
    const char * file = strrchr(path,'/');
    if (file)
        return file+1;
    else
        return path;
}

VOID RecordMemWrite(VOID * ip)
{

	if(!isVMstart)
		START_access[1]++;
	else if(!isVMinit)
		INIT_access[1]++;
	else if(isGCrunning)
		GC_access[1]++;
	else
		EXEC_access[1]++;
	i++;
}

VOID RecordMemRead(VOID * ip)
{

	if(!isVMstart)
		START_access[0]++;
	else if(!isVMinit)
		INIT_access[0]++;
	else if(isGCrunning)
		GC_access[0]++;
	else
		EXEC_access[0]++;
	i++;
}

VOID Routine(RTN rtn, VOID *v)
{
	RTN_Open(rtn);
	
	string image_name = StripPath(IMG_Name(SEC_Img(RTN_Sec(rtn))).c_str());
	log_file << "Image : "<< image_name << "\tFonction " <<  RTN_Name(rtn) << endl;
		
	for (INS ins = RTN_InsHead(rtn); INS_Valid(ins); ins = INS_Next(ins)){
	

		UINT32 memOperands = INS_MemoryOperandCount(ins);

		for (UINT32 memOp = 0; memOp < memOperands; memOp++)
		{
			if (INS_MemoryOperandIsRead(ins, memOp))
			{
				INS_InsertPredicatedCall(
				ins, IPOINT_BEFORE, (AFUNPTR)RecordMemRead,
				IARG_INST_PTR,
				IARG_END);
			}
			if (INS_MemoryOperandIsWritten(ins, memOp))
			{
				INS_InsertPredicatedCall(
				ins, IPOINT_BEFORE, (AFUNPTR)RecordMemWrite,
				IARG_INST_PTR,
				IARG_END);
			}
		}
	}

	RTN_Close(rtn);
}

VOID SyscallEntry(THREADID threadIndex, CONTEXT *ctxt, SYSCALL_STANDARD std, VOID *v)
{	
	ADDRINT id = PIN_GetSyscallNumber(ctxt, std);

	if( id != SYS_write)
		return;

	ADDRINT arg0 = PIN_GetSyscallArgument(ctxt, std, 0);

	if(arg0 != 1 && arg0 != 2)
		return;
		
	char *arg1 = reinterpret_cast<char *>(PIN_GetSyscallArgument(ctxt, std, 1));

	string app_out = string(arg1);
	if(app_out.size() < 2)
		return;

	if( !(app_out[0] == '!' && app_out[1] == '='))
		return;
	
	char op = app_out[2];
	int arg = atoi(&op);
	
	switch(arg){
		case VMINIT : isVMinit = true; break;
		case GCBEGIN : isGCrunning = true; break; 
		case GCEND : isGCrunning = false; break;
		case VMSTART : isVMstart = true;break;
	}
	/*
	cout << "Intercept a write syscall " << endl;
	cout << "Arg 0 = " << arg0 << endl;	
	cout << "Arg 1 = " << arg1;
	*/				
}



VOID Fini(INT32 code, VOID *v)
{
	log_file.close();

	cout << "NB Access\t" << i << endl;
	uint64_t dummy = START_access[0] + START_access[1]; 
	cout << "START\t" << dummy << "\t" << (double) dummy*100/(double)i << "%" << endl; 
	cout << "\tRead\t" << (double) START_access[0]*100/ dummy << "%" << "\tWrite\t" << (double) START_access[1]*100/ dummy << "%" << endl;

	dummy = INIT_access[0] + INIT_access[1]; 
	cout << "INIT\t" << dummy << "\t" << (double) dummy*100/(double)i << "%" << endl; 
	cout << "\tRead\t" << (double) INIT_access[0]*100/ dummy << "%" << "\tWrite\t" << (double) INIT_access[1]*100/ dummy << "%" << endl;

	dummy = GC_access[0] + GC_access[1]; 
	cout << "GC\t" << dummy << "\t" << (double) dummy*100/(double)i << "%" << endl; 
	cout << "\tRead\t" << (double) GC_access[0]*100/ dummy << "%" << "\tWrite\t" << (double) GC_access[1]*100/ dummy << "%" << endl;


	dummy = EXEC_access[0] + EXEC_access[1]; 
	cout << "EXEC\t" << dummy << "\t" << (double) dummy*100/(double)i << "%" << endl; 
	cout << "\tRead\t" << (double) EXEC_access[0]*100/ dummy << "%" << "\tWrite\t" << (double) EXEC_access[1]*100/ dummy << "%" << endl;
}

   
INT32 Usage()
{
    PIN_ERROR( "This Pintool evaluates the impact on the Garbage collector of the overall Java Application\n" 
              + KNOB_BASE::StringKnobSummary() + "\n");
    return -1;
}

int main(int argc, char *argv[])
{
	PIN_InitSymbols();
	if (PIN_Init(argc, argv))
		return Usage();
	
	i = 0;

	isVMstart = false, isVMinit = false, isGCrunning = false;
	GC_access = vector<uint64_t>(2,0);
	START_access = GC_access;
	INIT_access = GC_access;
	EXEC_access = GC_access;

	log_file.open("log.out");
	RTN_AddInstrumentFunction(Routine, 0);
	
	PIN_AddFiniFunction(Fini, 0);
	PIN_AddSyscallEntryFunction(SyscallEntry, 0);
	// Never returns
	PIN_StartProgram();

    return 0;
}
