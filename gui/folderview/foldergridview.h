#ifndef FOLDERGRIDVIEW_H
#define FOLDERGRIDVIEW_H

#include <QGraphicsWidget>

#include "gui/customwidgets/thumbnailview.h"
#include "gui/folderview/thumbnailgridwidget.h"
#include "gui/flowlayout.h"
#include "components/actionmanager/actionmanager.h"

class FolderGridView : public ThumbnailView
{
    Q_OBJECT
public:
    explicit FolderGridView(QWidget *parent = nullptr);

public slots:
    void show();
    void hide();

    void selectFirst();
    void selectLast();
    void selectIndex(int index);
    void pageUp();
    void pageDown();
    void zoomIn();
    void zoomOut();
private:
    FlowLayout *flowLayout;
    QGraphicsWidget holderWidget;
    QStringList allowedKeys;
    int selectedIndex, shiftedIndex;

private slots:
    void selectAbove();
    void selectBelow();
    void selectNext();
    void selectPrev();

protected:
    void resizeEvent(QResizeEvent *event);
    void addItemToLayout(ThumbnailWidget *widget, int pos);
    void removeItemFromLayout(int pos);
    void setupLayout();
    ThumbnailWidget *createThumbnailWidget();
    void updateLayout();
    void ensureSelectedItemVisible();
    void fitToContents();

    void keyPressEvent(QKeyEvent *event);
    void wheelEvent(QWheelEvent *event);

signals:

protected slots:
    void setThumbnailSize(int newSize);
};

#endif // FOLDERGRIDVIEW_H
