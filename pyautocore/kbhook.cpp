#include <stdio.h>

#include <windows.h>

#include "pythonutil.h"
#include "pyautocore.h"

static HHOOK key_hook = NULL;

using namespace pyauto;

// WH_KEYBOARD_LL �� SendInput �̊֌W:
//
//   KeyHookProc() �̂Ȃ��� SendInput() ���Ăяo���ƁA
//   KeyHookProc() ���l�X�g���ČĂяo�����B
//
LRESULT CALLBACK KeyHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	PythonUtil::GIL_Ensure gil_ensure;

	if(nCode<0)
	{
		LRESULT result = CallNextHookEx(key_hook, nCode, wParam, lParam);
		return result;
	}

	KBDLLHOOKSTRUCT * pkbdllhook = (KBDLLHOOKSTRUCT*)lParam;

	// �^�C���X�^���v���t�]���Ă��܂����ꍇ�́A�\�����Ȃ����Ƃ��N���Ă���
	if( g.last_key_time > pkbdllhook->time )
	{
		PythonUtil_Printf("Time stamp inversion happened.\n");
	}

	// Pyauto �� SendInput �ɂ���đ}�����ꂽ�L�[�C�x���g�̓X�N���v�g�ŏ������Ȃ��B
	// ���̃v���O�����̃L�[�t�b�N�ő}�����ꂽ�L�[�C�x���g�𖳎����Ȃ����߂ɁA�^�C���X�^���v���`�F�b�N����B
	// vkCode==0 �̃C�x���g�͓��ʈ������A�K�� Python �ŏ�������B
	if( pkbdllhook->flags & LLKHF_INJECTED 
     && g.last_key_time >= pkbdllhook->time
	 && pkbdllhook->vkCode )
	{
		LRESULT result = CallNextHookEx(key_hook, nCode, wParam, lParam);
		return result;
	}

	switch(wParam)
	{
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
		if(g.pyhook->keydown)
		{
			DWORD vk = pkbdllhook->vkCode;
			DWORD scan = pkbdllhook->scanCode;
			g.last_key_time = pkbdllhook->time;
			
			PyObject * pyarglist = Py_BuildValue("(ii)", vk, scan );
			PyObject * pyresult = PyEval_CallObject( g.pyhook->keydown, pyarglist );
			Py_DECREF(pyarglist);
			if(pyresult)
			{
				int result;
				if( pyresult==Py_None )
				{
					result = 0;
				}
				else
				{
					PyArg_Parse(pyresult,"i", &result );
				}
				Py_DECREF(pyresult);

				if(result)
				{
					return result;
				}
			}
			else
			{
				PyErr_Print();
			}
		}
		break;

	case WM_KEYUP:
	case WM_SYSKEYUP:
		if(g.pyhook->keyup)
		{
			DWORD vk = pkbdllhook->vkCode;
			DWORD scan = pkbdllhook->scanCode;
			g.last_key_time = pkbdllhook->time;
			
			PyObject * pyarglist = Py_BuildValue("(ii)", vk, scan );
			PyObject * pyresult = PyEval_CallObject( g.pyhook->keyup, pyarglist );
			Py_DECREF(pyarglist);
			if(pyresult)
			{
				int result;
				if( pyresult==Py_None )
				{
					result = 0;
				}
				else
				{
					PyArg_Parse(pyresult,"i", &result );
				}
				Py_DECREF(pyresult);

				if(result)
				{
					return result;
				}
			}
			else
			{
				PyErr_Print();
			}
		}
		break;
	}

	LRESULT result = CallNextHookEx(key_hook, nCode, wParam, lParam);
	return result;
}

void HookStart_Key()
{
	if(!key_hook)
	{
		PythonUtil_DebugPrintf("SetWindowsHookEx\n" );
		key_hook = SetWindowsHookEx( WH_KEYBOARD_LL, KeyHookProc, g.module_handle, 0 );
	}

	if( key_hook==NULL )
	{
		PythonUtil_DebugPrintf("SetWindowsHookEx failed : %x\n", GetLastError() );
	}
}

void HookEnd_Key()
{
	if(key_hook)
	{
		if( ! UnhookWindowsHookEx(key_hook) )
		{
			PythonUtil_DebugPrintf("UnhookWindowsHookEx(key) failed\n");
		}

		key_hook=NULL;
	}
}
