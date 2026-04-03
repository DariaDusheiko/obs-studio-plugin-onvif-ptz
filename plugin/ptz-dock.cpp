#include <QGridLayout>
#include <QPushButton>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

#include <obs-frontend-api.h>
#include <obs-module.h>

#include "ptz-core.h"

#define DOCK_ID "onvif_ptz_control_dock"

static QWidget *s_root = nullptr;

static QPushButton *make_dir_btn(const QString &label, const QString &bg, const QString &bg_pressed)
{
	auto *b = new QPushButton(label);
	b->setMinimumSize(72, 44);
	b->setStyleSheet(QStringLiteral("QPushButton { background-color: %1; color: #ffffff; font-weight: bold; "
					"border-radius: 8px; padding: 6px; }"
					"QPushButton:pressed { background-color: %2; }"
					"QPushButton:hover { border: 2px solid #ffffff; }")
				 .arg(bg, bg_pressed));
	return b;
}

static void wire_hold(QPushButton *b, int direction)
{
	QObject::connect(b, &QPushButton::pressed, [direction]() { ptz_start_motion(direction); });
	QObject::connect(b, &QPushButton::released, []() { ptz_stop_motion(); });
}

extern "C" void ptz_dock_try_register(void)
{
	if (s_root)
		return;

	auto *w = new QWidget();
	auto *outer = new QVBoxLayout(w);
	outer->setContentsMargins(12, 12, 12, 12);

	auto *grid = new QGridLayout();
	grid->setSpacing(8);
	grid->setColumnStretch(0, 1);
	grid->setColumnStretch(1, 1);
	grid->setColumnStretch(2, 1);

	QPushButton *up = make_dir_btn(QStringLiteral(u8"▲  UP"), QStringLiteral("#2e7d32"), QStringLiteral("#1b5e20"));
	QPushButton *down = make_dir_btn(QStringLiteral(u8"▼  DOWN"), QStringLiteral("#2e7d32"), QStringLiteral("#1b5e20"));
	QPushButton *left = make_dir_btn(QStringLiteral(u8"◀  LEFT"), QStringLiteral("#1565c0"), QStringLiteral("#0d47a1"));
	QPushButton *right = make_dir_btn(QStringLiteral(u8"RIGHT  ▶"), QStringLiteral("#1565c0"), QStringLiteral("#0d47a1"));

	auto *stop = new QPushButton(QStringLiteral(u8"■  STOP"));
	stop->setMinimumSize(72, 44);
	stop->setStyleSheet(QStringLiteral("QPushButton { background-color: #c62828; color: #ffffff; font-weight: bold; "
					   "border-radius: 8px; padding: 6px; font-size: 13px; }"
					   "QPushButton:pressed { background-color: #8e0000; }"
					   "QPushButton:hover { border: 2px solid #ffffff; }"));

	wire_hold(up, 1);
	wire_hold(down, 2);
	wire_hold(left, 3);
	wire_hold(right, 4);
	QObject::connect(stop, &QPushButton::clicked, []() { ptz_stop_motion(); });

	grid->addWidget(up, 0, 1);
	grid->addWidget(left, 1, 0);
	grid->addWidget(stop, 1, 1);
	grid->addWidget(right, 1, 2);
	grid->addWidget(down, 2, 1);

	outer->addLayout(grid);
	outer->addStretch();

	if (!obs_frontend_add_dock_by_id(DOCK_ID, "PTZ Control", w)) {
		blog(LOG_WARNING, "[onvif_ptz] dock id already in use; skip PTZ dock");
		delete w;
		return;
	}

	s_root = w;
	blog(LOG_INFO, "[onvif_ptz] PTZ control dock registered");
}

extern "C" void ptz_dock_unregister(void)
{
	if (!s_root)
		return;
	obs_frontend_remove_dock(DOCK_ID);
	s_root = nullptr;
}
