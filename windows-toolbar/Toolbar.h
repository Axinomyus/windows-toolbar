#pragma once
#include <windows.h>

void UpdateToolbarPosition();
void UpdateMinimizedToolbarPosition();
void ShowToolbar(bool show);
double GetEaseOutCubic(double t);
double GetAnimationProgress();