#ifndef NOSCROLLCOMBOBOX_H
#define NOSCROLLCOMBOBOX_H

#include <QComboBox>

class NoScrollComboBox : public QComboBox
{
    Q_OBJECT

public:
    explicit NoScrollComboBox(QWidget *parent = nullptr);
    void wheelEvent(QWheelEvent *event);
    void setTextItem(const QString &text);

private:
};

#endif // NOSCROLLCOMBOBOX_H
