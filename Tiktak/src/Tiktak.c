// Tiktak.cpp : Defines the entry point for the application.
//
#define COBJMACROS

#include "framework.h"
#include "Tiktak.h"

#define IDT_TIMER (WM_USER + 0x0001)
#define UM_RESET (WM_USER + 0x0002)
#define IDM_ABOUTBOX (WM_USER + 0x0003)

#define TOI(chr) (chr - 48)
#define TOC(i) (i + 48)
#define NULL_TIMER(bf) (bf[0] == 48 && bf[1] == 48 && bf[3] == 48 && bf[4] == 48)

// Global Variables:
HINSTANCE hInst;                                // current instance
HWND hEditTime;
CHAR timeBuffer[6];
BOOL bStarted = FALSE;
BOOL bReset = FALSE;
BYTE hours;
BYTE minutes_left;
UINT minutes = 0;
//UINT seconds;
ITaskbarList3* pTaskBar;

INT_PTR CALLBACK    DialogProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY WinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    DialogBox(hInstance, MAKEINTRESOURCE(IDD_TIKTAK), NULL, DialogProc);

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_TIKTAK));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}

INT_PTR CALLBACK DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
    {
        hEditTime = GetDlgItem(hDlg, IDC_EDIT_TIME);

        /* changing font */
        HDC hDC = CreateCompatibleDC(GetDC(HWND_DESKTOP));

        static const int points_per_inch = 72;
        int pixels_per_inch = GetDeviceCaps(hDC, LOGPIXELSY);
        int pixels_height = -(32 * pixels_per_inch / points_per_inch);

        HFONT font = CreateFontA(pixels_height, 0, 0, 0, 0,
            FALSE, FALSE, FALSE,
            ANSI_CHARSET,
            OUT_OUTLINE_PRECIS,
            CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY,
            VARIABLE_PITCH | FF_SWISS,
            "Segoe UI");

        SendMessage(hEditTime, WM_SETFONT, (WPARAM)font, TRUE);

        /* updating window */
        SetWindowText(hEditTime, TEXT("00:05"));

        break;
    }

    case WM_COMMAND:

        switch (LOWORD(wParam))
        {

        case IDOK:

            if (bStarted == FALSE)
            {
                GetWindowTextA(hEditTime, &timeBuffer, 6);

                hours = TOI(timeBuffer[1]) + (TOI(timeBuffer[0]) * 10);
                minutes = minutes_left = (hours * 60) + (TOI(timeBuffer[4]) + (TOI(timeBuffer[3]) * 10));
                //seconds = (hours * 3600) + (minutes_left * 60);

                SetTimer(hDlg, IDT_TIMER, 60000, (TIMERPROC)NULL);
                SetWindowText(hDlg, &timeBuffer);

                /* taskbar progress init */
                CoInitializeEx(NULL, COINITBASE_MULTITHREADED);
                CoCreateInstance(&CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER, &IID_ITaskbarList3, &pTaskBar);

                if (pTaskBar == NULL)
                {
                    CoUninitialize();
                    ExitProcess(0);
                }

                ITaskbarList3_HrInit(pTaskBar);
                ITaskbarList3_SetProgressState(pTaskBar, hDlg, TBPF_NORMAL);

                ShowWindow(hDlg, SW_MINIMIZE);
                SetWindowText(GetDlgItem(hDlg, IDOK), TEXT("Stop"));

                bStarted = TRUE;
            }
            else
            {
                SendMessage(hDlg, UM_RESET, NULL, NULL);
            }

            break;

        case IDM_ABOUTBOX:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hDlg, About);
            break;
        }

        break;

    case IDM_ABOUTBOX:
        DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hDlg, About);
        break;

    case WM_TIMER:
    {
        if (wParam == IDT_TIMER)
        {
            //--seconds;
            //hours = seconds / 3600;
            //minutes_left = (seconds / 60) % 60;

            --minutes_left;
            hours = minutes_left / 60;

            timeBuffer[0] = TOC(hours / 10);
            timeBuffer[1] = TOC(hours % 10);
            timeBuffer[3] = TOC(minutes_left % 60 / 10);
            timeBuffer[4] = TOC(minutes_left % 60 % 10);

            SetWindowText(hEditTime, &timeBuffer);
            SetWindowText(hDlg, &timeBuffer);
            ITaskbarList3_SetProgressValue(pTaskBar, hDlg, minutes - minutes_left, minutes);

            if (NULL_TIMER(timeBuffer))
            {
                MessageBeep(MB_OK);
                SendMessage(hDlg, UM_RESET, NULL, NULL);
            }
        }

        break;
    }

    case WM_SIZE:
        if (wParam == SIZE_RESTORED)
        {
            if (bReset == TRUE)
            {
                ITaskbarList3_SetProgressState(pTaskBar, hDlg, TBPF_NOPROGRESS);
                ITaskbarList3_Release(pTaskBar);
                CoUninitialize();
                SetWindowText(hDlg, TEXT("Tiktak"));
                bReset = FALSE;
            }
        }
        break;

    case WM_LBUTTONDOWN:
        ShowWindow(hDlg, SW_MINIMIZE);
        break;

    case WM_RBUTTONDOWN:
    {
        HMENU hPopupMenu = CreatePopupMenu();
        InsertMenu(hPopupMenu, 0, MF_BYPOSITION | MF_STRING, IDM_ABOUTBOX, TEXT("About"));
        InsertMenu(hPopupMenu, 1, MF_SEPARATOR, 0, NULL);
        InsertMenu(hPopupMenu, 2, MF_BYPOSITION | MF_STRING, WM_DESTROY, TEXT("Exit"));

        POINT point;
        point.x = LOWORD(lParam);
        point.y = HIWORD(lParam);
        ClientToScreen(hDlg, &point);

        //GetCursorPos(&point);



        SetForegroundWindow(hDlg);
        TrackPopupMenu(hPopupMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, point.x, point.y, 0, hDlg, NULL);

        //DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hDlg, About);
    }
        break;

    case WM_CLOSE:
        EndDialog(hDlg, LOWORD(wParam));
        DestroyWindow(hDlg);
        break;

    case WM_DESTROY:
        SendMessage(hDlg, UM_RESET, NULL, NULL);
        PostQuitMessage(0);
        break;

    case UM_RESET:
    {
        SetWindowText(GetDlgItem(hDlg, IDOK), TEXT("Start"));
        KillTimer(hDlg, IDT_TIMER);
        bStarted = FALSE;

        if (IsIconic(hDlg))
        {
            ITaskbarList3_SetProgressState(pTaskBar, hDlg, TBPF_ERROR);
            bReset = TRUE;
            break;
        }

        ITaskbarList3_Release(pTaskBar);
        CoUninitialize();
        SetWindowText(hDlg, TEXT("Tiktak"));
        break;
    }

    default:
        return (INT_PTR)FALSE;
    }

    return (INT_PTR)TRUE;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
