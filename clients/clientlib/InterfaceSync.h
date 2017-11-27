#pragma once

#include <QHash>

class QString;
class QLineEdit;
class QLabel;
class QDateEdit;
class QDateTimeEdit;
class QTimeEdit;
class QSpinBox;
class QComboBox;
class QAbstractItemView;
class QCheckBox;
class QRadioButton;
class QPushButton;
class QWidget;
class QVariant;

namespace Sportsed {
namespace Client {
class RecordObject;
class AbstractRecordModel;

namespace ViewSync {
void sync(QAbstractItemView *view, const int role, QLabel *widget);
void sync(QAbstractItemView *view, const int role, QLineEdit *widget);
void sync(QAbstractItemView *view, const int role, QDateTimeEdit *widget);
void sync(QAbstractItemView *view, const int role, QSpinBox *widget);
void sync(QAbstractItemView *view, const int role, QComboBox *widget);
void sync(QAbstractItemView *view, const int role, QCheckBox *widget);
void sync(QAbstractItemView *view, const int role, const QHash<QRadioButton *, QVariant> &widgets);

void sync(QComboBox *view, const int role, QLabel *widget);
void sync(QComboBox *view, const int role, QLineEdit *widget);
void sync(QComboBox *view, const int role, QDateTimeEdit *widget);
void sync(QComboBox *view, const int role, QSpinBox *widget);
void sync(QComboBox *view, const int role, QComboBox *widget);
void sync(QComboBox *view, const int role, QCheckBox *widget);
void sync(QComboBox *view, const int role, const QHash<QRadioButton *, QVariant> &widget);

void buttons(QAbstractItemView *view, QPushButton *add, QPushButton *remove);
void buttons(QComboBox *view, QPushButton *add, QPushButton *remove);
}

namespace ItemSync {
void sync(RecordObject *obj, const QString &field, QLabel *widget);
void sync(RecordObject *obj, const QString &field, QLineEdit *widget);
void sync(RecordObject *obj, const QString &field, QDateTimeEdit *widget);
void sync(RecordObject *obj, const QString &field, QSpinBox *widget);
void sync(RecordObject *obj, const QString &field, QComboBox *widget);
}

}
}
