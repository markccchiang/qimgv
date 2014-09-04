#include "imageviewer.h"

#include <qgraphicseffect.h>

enum WindowResizePolicy
{
    NORMAL,
    WIDTH,
    ALL,
    FREE
};

class ImageViewerPrivate
{
public:
    ImageViewerPrivate(ImageViewer* qq);
    ~ImageViewerPrivate();
    void centerVertical();
    void centerHorizontal();
    void setImage(Image* image);
    void setScale(float scale);
    void scaleImage();
    float scale() const;
    bool scaled() const;

    Image* img;
    QTimer animationTimer;
    QImageReader imageReader;
    QImage image, imageScaled;
    QPoint cursorMovedDistance;
    QRect drawingRect;
    QSize shrinkSize;
    MapOverlay *mapOverlay;
    InfoOverlay *infoOverlay;
    ControlsOverlay *controlsOverlay;
    ImageViewer* q;

    WindowResizePolicy resizePolicy;

    int freeSpaceLeft;
    int freeSpaceBottom;
    int freeSpaceRight;
    int freeSpaceTop;

    bool isDisplaying;

    static const float maxScale = 0.10;
    static const float minScale = 3.0;
    static const float zoomStep = 0.1;

private:
    float currentScale;
};

ImageViewerPrivate::ImageViewerPrivate(ImageViewer* qq)
    : q(qq),
      shrinkSize(),
      currentScale(1.0),
      resizePolicy(NORMAL),
      img(NULL),
      isDisplaying(false)
{
    infoOverlay = new InfoOverlay(q);
    mapOverlay = new MapOverlay(q);
    controlsOverlay = new ControlsOverlay(q);
    infoOverlay->hide();
    controlsOverlay->hide();
}

ImageViewerPrivate::~ImageViewerPrivate()
{
    delete img;
}

bool ImageViewerPrivate::scaled() const
{
    return scale() != 1.0;
}

void ImageViewerPrivate::setImage(Image* i) {
    if (img!=NULL) {
        if(img->getType()==GIF) {
            img->getMovie()->stop();
            q->disconnect(img->getMovie(), SIGNAL(frameChanged(int)), q, SLOT(onAnimation()));
        }
        delete img;
        img = NULL;
    }

    if(i->getType()==NONE) {
        //empty or corrupted image
        image.load(":/images/res/error.png");
        isDisplaying = false;
    }
    else {
        //ok, proceeding
        img = i;
        if(img->getType() == STATIC) {
            image = *img->getImage();
        }
        else if (img->getType() == GIF) {
            img->getMovie()->jumpToFrame(0);
            image = img->getMovie()->currentImage();
            q->connect(img->getMovie(), SIGNAL(frameChanged(int)), q, SLOT(onAnimation()));
            img->getMovie()->start();
        }

        isDisplaying = true;
        emit q->imageChanged();
    }
    drawingRect = image.rect();
    if(resizePolicy == FREE)
        resizePolicy = NORMAL;
}

float ImageViewerPrivate::scale() const
{
    return currentScale;
}

void ImageViewerPrivate::setScale(float scale)
{
    currentScale = scale;
    QSize sz;
    sz = image.size();
    sz = sz.scaled(sz * scale, Qt::KeepAspectRatio);
    drawingRect.setSize(sz);
}

void ImageViewerPrivate::scaleImage()
{
    if(scaled()) {
        imageScaled = image.scaled(drawingRect.size(),Qt::KeepAspectRatio);
    }
}

void ImageViewerPrivate::centerHorizontal()
{
    QPoint point((q->width() - drawingRect.width()) / 2, drawingRect.y());
    drawingRect.moveTo(point);
}

void ImageViewerPrivate::centerVertical()
{
    QPoint point(drawingRect.x(), (q->height() - drawingRect.height()) / 2);
    drawingRect.moveTo(point);
}

ImageViewer::ImageViewer(): QWidget(), d(new ImageViewerPrivate(this))
{
    d->image.load(":/images/res/logo.png");
    d->drawingRect = d->image.rect();
    fitDefault();
}

ImageViewer::ImageViewer(QWidget* parent): QWidget(parent),
d(new ImageViewerPrivate(this))
{
    d->image.load(":/images/res/logo.png");
    d->drawingRect = d->image.rect();
}

ImageViewer::ImageViewer(QWidget* parent, Image* image) : QWidget(parent),
d(new ImageViewerPrivate(this))
{
    setImage(image);
}

ImageViewer::~ImageViewer()
{
    delete d;
}

void ImageViewer::setImage(Image* image)
{
    d->setImage(image);
    fitDefault();
}

void ImageViewer::onAnimation()
{
    d->image = d->img->getMovie()->currentImage();
    d->scaleImage();
    update();
}

void ImageViewer::paintEvent(QPaintEvent* event)
{
    QColor bgColor = QColor(0,0,0,255);
    QPainter painter(this);
    painter.setPen(bgColor);
    painter.setBrush(Qt::SolidPattern);
    painter.drawRect(QRect(0,0,this->width(),this->height()));

    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    int time = clock();
    if(d->scaled()) {
        painter.drawImage(d->drawingRect, d->imageScaled);
    }
    else {
        painter.drawImage(d->drawingRect, d->image);
    }
    qDebug() << "render time: " << clock() - time;
}

void ImageViewer::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        this->setCursor(QCursor(Qt::ClosedHandCursor));
        d->cursorMovedDistance = event->pos();
    }
}

void ImageViewer::mouseMoveEvent(QMouseEvent* event)
{
    if (event->buttons() & Qt::LeftButton) {
        d->cursorMovedDistance -= event->pos();

        int left = d->drawingRect.x() - d->cursorMovedDistance.x();
        int top = d->drawingRect.y() - d->cursorMovedDistance.y();
        int right = left + d->drawingRect.width();
        int bottom = top + d->drawingRect.height();

        if (left < 0 && right > size().width())
            d->drawingRect.moveLeft(left);

        if (top < 0 && bottom > size().height())
            d->drawingRect.moveTop(top);

        d->cursorMovedDistance = event->pos();
        update();
        d->mapOverlay->updateMap(size(),d->drawingRect);
    }
}

void ImageViewer::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        this->setCursor(QCursor(Qt::ArrowCursor));
        d->cursorMovedDistance = event->pos();
    }
}

void ImageViewer::fitWidth()
{
    float scale = (float) width() / d->image.width();
    d->drawingRect.setX(0);
    d->setScale(scale);
    if(d->drawingRect.height()<=height()) {
        QPoint point(0, (height() - d->drawingRect.height()) / 2);
        d->drawingRect.moveTo(point);
    }
    else
        d->drawingRect.moveTop(0);
    if(d->scaled()) {
        d->imageScaled = d->image.scaled(d->drawingRect.size(),Qt::KeepAspectRatio);
    }
    update();
}

void ImageViewer::fitHorizontal()
{
    float scale = (float) width() / d->image.width();
    d->drawingRect.setX(0);
    d->setScale(scale);
    d->centerVertical();
}

void ImageViewer::fitVertical()
{
    float scale = (float) height() / d->image.height();
    d->drawingRect.setY(0);
    d->setScale(scale);
    d->centerHorizontal();
}

void ImageViewer::fitAll()
{
    bool h = d->image.height() < this->height();
    bool w = d->image.width() < this->width();
    if(h && w) {
        fitOriginal();
    }
    else {
        float widgetAspect = (float) height() / width();
        float drawingAspect = (float) d->drawingRect.height() /
                d->drawingRect.width();
        if(widgetAspect < drawingAspect)
            fitVertical();
        else
            fitHorizontal();
    }
    if(d->scaled()) {
        d->imageScaled = d->image.scaled(d->drawingRect.size(),Qt::KeepAspectRatio);
    }
    update();
}

void ImageViewer::fitOriginal()
{
   setScale(1.0);
   centerImage();
   update();
}

void ImageViewer::fitDefault() {
    switch(d->resizePolicy) {
        case NORMAL: fitOriginal(); break;
        case WIDTH: fitWidth(); break;
        case ALL: fitAll(); break;
        default: centerImage(); break;
    }
    d->scaleImage();
    d->mapOverlay->updateMap(size(),d->drawingRect);
}

void ImageViewer::centerImage() {
    int left = d->drawingRect.left()<0?0:d->drawingRect.left();
    int right = d->drawingRect.right()<0?0:d->drawingRect.right();
    int top = d->drawingRect.top()<0?0:d->drawingRect.top();
    int bottom = d->drawingRect.bottom()<0?0:d->drawingRect.bottom();
    int spaceLeft = left - rect().left();
    int spaceTop = top - rect().top();
    int spaceRight = rect().right() - right;
    int spaceBottom = rect().bottom() - bottom;

    if (spaceLeft < 0 && spaceRight > 0)
        d->drawingRect.translate(spaceRight, 0);
    else if (spaceLeft > 0 || spaceRight > 0)
        d->centerHorizontal();

    if (spaceTop < 0 && spaceBottom > 0)
        d->drawingRect.translate(0, spaceBottom);
    else if (spaceTop > 0 || spaceBottom > 0)
        d->centerVertical();
}

void ImageViewer::slotFitNormal() {
    d->resizePolicy = NORMAL;
    fitDefault();
}

void ImageViewer::slotFitWidth() {
    d->resizePolicy = WIDTH;
    fitDefault();
}

void ImageViewer::slotFitAll() {
    d->resizePolicy = ALL;
    fitDefault();
}

void ImageViewer::resizeEvent(QResizeEvent* event)
{
    resize(event->size());
    fitDefault();
    d->scaleImage();
    d->mapOverlay->updateMap(size(),d->drawingRect);
    d->controlsOverlay->updateSize();
    d->mapOverlay->updatePosition();
}

void ImageViewer::mouseDoubleClickEvent(QMouseEvent *event) {
    emit sendDoubleClick();
}

void ImageViewer::slotZoomIn() {
    float possibleScale = d->scale() + d->zoomStep;
    if (possibleScale <= d->minScale) {
        d->setScale(possibleScale);
    }
    else {
        d->setScale(d->minScale);
    }
    d->centerHorizontal();
    d->centerVertical();
    d->resizePolicy = FREE;
    d->scaleImage();
    update();
    d->mapOverlay->updateMap(size(),d->drawingRect);
}

void ImageViewer::slotZoomOut() {
    float possibleScale = d->scale() - d->zoomStep;
    if (possibleScale >= d->maxScale) {
        d->setScale(possibleScale);
    }
    else {
        d->setScale(d->maxScale);
    }
    d->centerHorizontal();
    d->centerVertical();
    d->resizePolicy = FREE;
    d->scaleImage();
    update();
    d->mapOverlay->updateMap(size(),d->drawingRect);
}

Image* ImageViewer::getImage() const {
    return d->img;
}

void ImageViewer::setScale(float scale)
{
    d->setScale(scale);
}

void ImageViewer::slotSetInfoString(QString info) {
    d->infoOverlay->setText(info);
}

void ImageViewer::slotShowInfo(bool x) {
    x?d->infoOverlay->show():d->infoOverlay->hide();
}

void ImageViewer::slotShowControls(bool x) {
    x?d->controlsOverlay->show():d->controlsOverlay->hide();
}

ControlsOverlay* ImageViewer::getControls() {
    return d->controlsOverlay;
}

bool ImageViewer::isDisplaying() {
    return d->isDisplaying;
}