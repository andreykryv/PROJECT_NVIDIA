////////////////////////////////////////////////////////////////////////////////
// visualization/colorscheme.cpp — реализация цветовых схем
//
// RainbowScheme::getColor():
//   QColor::fromHsvF(value * 0.667, 0.9, 0.9) — 0.667 = 240°/360°
//   Для state==Comparing: увеличить saturation до 1.0, value до 1.0 (ярче).
//   Для state==Sorted: перекрыть зелёным QColor(50, 200, 100).
//   Для state==Pivot: синий QColor(30, 120, 255).
//   Для state==Swapping: красный QColor(255, 60, 60).
//
// HeatmapScheme::getColor():
//   Градиент через QLinearGradient с 5 ключевыми цветами.
//   Интерполируется линейно между ближайшими точками.
//
// create(): switch(type) возвращает new XxxScheme.
////////////////////////////////////////////////////////////////////////////////
