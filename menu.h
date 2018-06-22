void DisplayFlashStatusUpdate(unsigned int message);
void MainMenu(void);
#ifndef FSCK
void InitProgressScreen(int label);
void DrawDiskSurfScanningScreen(int PercentageComplete, unsigned int SecondsRemaining, unsigned int NumBadSectors);
void DrawDiskZeroFillingScreen(int PercentageComplete, unsigned int SecondsRemaining);
void DrawDiskScanningScreen(int PercentageComplete, unsigned int SecondsRemaining);
void DrawDiskOptimizationScreen(int PercentageComplete, int TotalPercentageComplete, unsigned int SecondsRemaining);
int GetBadSectorAction(u32 BadSectorLBA);
#endif
void DisplayScanCompleteResults(unsigned int ErrorsFound, unsigned int ErrorsFixed);
void RedrawLoadingScreen(unsigned int frame);
