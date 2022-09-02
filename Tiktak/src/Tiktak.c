// Tiktak.cpp : Defines the entry point for the application.
//
#define COBJMACROS

#include "framework.h"
#include "Tiktak.h"

// resourse and message ids
#define IDT_TIMER (WM_USER + 0x0001)
#define UM_RESET (WM_USER + 0x0002)
#define IDM_ABOUTBOX (WM_USER + 0x0003)

// utility
#define TOI(chr) (chr - 48)
#define TOC(i) (i + 48)
#define NULL_TIMER(bf) (bf[0] == 48 && bf[1] == 48 && bf[3] == 48 && bf[4] == 48)

// props
#define EDIT_TIME_LENGTH_MAX 8

struct App {

	HINSTANCE hInst;
	HWND hEditTime;
	ITaskbarList3* pTaskBar;

	BOOL bStarted;
	BOOL bReset;
	BYTE hours;
	BYTE minutes_left;
	UINT minutes;
	TCHAR timeBuffer[EDIT_TIME_LENGTH_MAX];
};

INT_PTR CALLBACK    DialogProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY WinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPTSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    struct App app = { .hInst = hInstance };
    DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_TIKTAK), NULL, DialogProc, (LPARAM)&app);

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
    static struct App* app;

    switch (uMsg)
    {
    case WM_INITDIALOG:
    {
        app = (struct App*)lParam;
        app->hEditTime = GetDlgItem(hDlg, IDC_EDIT_TIME);

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

        SendMessage(app->hEditTime, WM_SETFONT, (WPARAM)font, TRUE);

        /* updating window */
        SetWindowText(app->hEditTime, TEXT("00:05"));

        break;
    }

    case WM_COMMAND:

        switch (LOWORD(wParam))
        {

        case IDOK:

            if (app->bStarted == FALSE)
            {
                GetWindowTextA(app->hEditTime, app->timeBuffer, EDIT_TIME_LENGTH_MAX);

                if (app->timeBuffer[2] != ':') {
                    MessageBox(hDlg, TEXT("Invalid format"), TEXT("Warning"), MB_ICONEXCLAMATION | MB_OK);
					SendMessage(hDlg, UM_RESET, (WPARAM)NULL, (LPARAM)NULL);
                    break;
                }

                app->hours = TOI(app->timeBuffer[1]) + (TOI(app->timeBuffer[0]) * 10);
                app->minutes = app->minutes_left = (app->hours * 60) + (TOI(app->timeBuffer[4]) + (TOI(app->timeBuffer[3]) * 10));

                if (app->minutes > 59 || app->timeBuffer[5] != '\0') {
                    MessageBox(hDlg, TEXT("Invalid format"), TEXT("Warning"), MB_ICONEXCLAMATION | MB_OK);
					SendMessage(hDlg, UM_RESET, (WPARAM)NULL, (LPARAM)NULL);
                    break;
                }

                SetTimer(hDlg, IDT_TIMER, 60000, (TIMERPROC)NULL);
                SetWindowText(hDlg, app->timeBuffer);

                /* taskbar progress init */
                if (FAILED(CoInitializeEx(NULL, COINITBASE_MULTITHREADED)))
                    (void)0;

                if (FAILED(CoCreateInstance(&CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER, &IID_ITaskbarList3, &app->pTaskBar)))
                    (void)0;

                if (app->pTaskBar == NULL)
                {
                    CoUninitialize();
                    ExitProcess(0);
                }

                ITaskbarList3_HrInit(app->pTaskBar);
                ITaskbarList3_SetProgressState(app->pTaskBar, hDlg, TBPF_NORMAL);

                ShowWindow(hDlg, SW_MINIMIZE);
                SetWindowText(GetDlgItem(hDlg, IDOK), TEXT("Stop"));

                app->bStarted = TRUE;
            }
            else
            {
                SendMessage(hDlg, UM_RESET, (WPARAM)NULL, (LPARAM)NULL);
            }

            break;

        case IDM_ABOUTBOX:
            DialogBox(app->hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hDlg, About);
            break;
        }

        break;

    case IDM_ABOUTBOX:
        DialogBox(app->hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hDlg, About);
        break;

    case WM_TIMER:
    {
        if (wParam == IDT_TIMER)
        {
            --app->minutes_left;
            app->hours = app->minutes_left / 60;

            app->timeBuffer[0] = TOC(app->hours / 10);
            app->timeBuffer[1] = TOC(app->hours % 10);
            app->timeBuffer[3] = TOC(app->minutes_left % 60 / 10);
            app->timeBuffer[4] = TOC(app->minutes_left % 60 % 10);

            SetWindowText(app->hEditTime, app->timeBuffer);
            SetWindowText(hDlg, app->timeBuffer);
            ITaskbarList3_SetProgressValue(app->pTaskBar, hDlg, app->minutes - app->minutes_left, app->minutes);

            if (NULL_TIMER(app->timeBuffer))
            {
                MessageBeep(MB_OK);
                SendMessage(hDlg, UM_RESET, (WPARAM)NULL, (LPARAM)NULL);
            }
        }

        break;
    }

    case WM_SIZE:
        if (wParam == SIZE_RESTORED)
        {
            if (app->bReset == TRUE)
            {
                ITaskbarList3_SetProgressState(app->pTaskBar, hDlg, TBPF_NOPROGRESS);
                ITaskbarList3_Release(app->pTaskBar);
                CoUninitialize();
                SetWindowText(hDlg, TEXT("Tiktak"));
                app->bReset = FALSE;
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

        POINT point = { 0 };
        point.x = LOWORD(lParam);
        point.y = HIWORD(lParam);
        ClientToScreen(hDlg, &point);

        SetForegroundWindow(hDlg);
        TrackPopupMenu(hPopupMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, point.x, point.y, 0, hDlg, NULL);
    }
        break;

    case WM_CLOSE:
        EndDialog(hDlg, LOWORD(wParam));
        DestroyWindow(hDlg);
        break;

    case WM_DESTROY:
        SendMessage(hDlg, UM_RESET, (WPARAM)NULL, (LPARAM)NULL);
        PostQuitMessage(0);
        break;

    case UM_RESET:
    {
        SetWindowText(GetDlgItem(hDlg, IDOK), TEXT("Start"));
        KillTimer(hDlg, IDT_TIMER);
        app->bStarted = FALSE;

        if (IsIconic(hDlg))
        {
            ITaskbarList3_SetProgressState(app->pTaskBar, hDlg, TBPF_ERROR);
            app->bReset = TRUE;
            break;
        }

        if (app->pTaskBar != NULL) {
			ITaskbarList3_Release(app->pTaskBar);
            app->pTaskBar = NULL;
        }

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
