#include "gamescene.h"
#include "utils.h"
#include "pixmapmanager.h"
#include <QKeyEvent>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsPixmapItem>
#include <QGraphicsLineItem>
#include <QDebug>
#include <QDir>
#include <QPainter>
#include <QApplication>

GameScene::GameScene(QObject *parent)
    : QGraphicsScene(parent), nMapWidth(32), nMapHeight(32), fPlayerX(14.7f), fPlayerY(8), fPlayerA(-3.14159f / 2.0f), fFOV(3.14159f / 4.0f),
    fDepth(16.0f), fSpeed(.5f), fDepthBuffer(nullptr), m_brickWallPng(":/res/sprites/wall64.png")
{
    onUserCreate();
}

GameScene::~GameScene()
{
    delete[] fDepthBuffer;
}

void GameScene::loop()
{
    m_deltaTime = m_elapsedTimer.elapsed();
    m_elapsedTimer.restart();

    m_loopTime += m_deltaTime;
    while(m_loopTime > m_loopSpeed)
    {
        m_loopTime -= m_loopSpeed;
        handlePlayerInput();
        QImage image = QImage(SCREEN::LOGICAL_SIZE.width(), SCREEN::LOGICAL_SIZE.height(), QImage::Format_RGB32);
        image.fill(Qt::yellow);

        for(int x = 0; x < SCREEN::LOGICAL_SIZE.width(); x += 1)
        {
            float fRayAngle = (fPlayerA - fFOV / 2.0f) + ((float)x / (float)SCREEN::LOGICAL_SIZE.width()) * fFOV;

            float fStepSize = 0.1f;
            float fDistanceToWall = 0.0f; //

            bool bHitWall = false;

            float fEyeX = sinf(fRayAngle);
            float fEyeY = cosf(fRayAngle);

            float fSampleX = 0.0f;

            // Incrementally cast ray from player, along ray angle, testing for
            // intersection with a block
            while (!bHitWall && fDistanceToWall < fDepth)
            {
                fDistanceToWall += fStepSize;
                int nTestX = (int)(fPlayerX + fEyeX * fDistanceToWall);
                int nTestY = (int)(fPlayerY + fEyeY * fDistanceToWall);

                // Test if ray is out of bounds
                if (nTestX < 0 || nTestX >= nMapWidth || nTestY < 0 || nTestY >= nMapHeight)
                {
                    bHitWall = true;			// Just set distance to maximum depth
                    fDistanceToWall = fDepth;
                }
                else
                {
                    // Ray is inbounds so test to see if the ray cell is a wall block
                    if (map[nTestX * nMapWidth + nTestY] == '#')
                    {
                        // Ray has hit wall
                        bHitWall = true;

                        // Determine where ray has hit wall. Break Block boundary
                        // int 4 line segments
                        float fBlockMidX = (float)nTestX + 0.5f;
                        float fBlockMidY = (float)nTestY + 0.5f;

                        float fTestPointX = fPlayerX + fEyeX * fDistanceToWall;
                        float fTestPointY = fPlayerY + fEyeY * fDistanceToWall;

                        float fTestAngle = atan2f((fTestPointY - fBlockMidY), (fTestPointX - fBlockMidX));

                        if (fTestAngle >= -3.14159f * 0.25f && fTestAngle < 3.14159f * 0.25f)
                            fSampleX = fTestPointY - (float)nTestY;
                        if (fTestAngle >= 3.14159f * 0.25f && fTestAngle < 3.14159f * 0.75f)
                            fSampleX = fTestPointX - (float)nTestX;
                        if (fTestAngle < -3.14159f * 0.25f && fTestAngle >= -3.14159f * 0.75f)
                            fSampleX = fTestPointX - (float)nTestX;
                        if (fTestAngle >= 3.14159f * 0.75f || fTestAngle < -3.14159f * 0.75f)
                            fSampleX = fTestPointY - (float)nTestY;
                    }
                }
            }

            // Calculate distance to ceiling and floor
            int nCeiling = (float)(SCREEN::LOGICAL_SIZE.height() / 2.0) - SCREEN::LOGICAL_SIZE.height() / ((float)fDistanceToWall);
            int nFloor = SCREEN::LOGICAL_SIZE.height() - nCeiling;

            // Update Depth Buffer
            fDepthBuffer[x] = fDistanceToWall;

            for (int y = 0; y < SCREEN::LOGICAL_SIZE.height(); y++)
            {
                // Each Row
                if (y <= nCeiling) {
                    if(x >= 0 && x < SCREEN::LOGICAL_SIZE.width() && y >= 0 && y < SCREEN::LOGICAL_SIZE.height())
                    image.setPixelColor(x, y, Qt::black);
                }
                else if (y > nCeiling && y <= nFloor)
                {
                    // Draw Wall
                    if (fDistanceToWall < fDepth)
                    {
                        float fSampleY = ((float)y - (float)nCeiling) / ((float)nFloor - (float)nCeiling);
                        float imgX = fSampleX * (m_brickWallPng.width() - 1);
                        float imgY = fSampleY * (m_brickWallPng.height() - 1);

                        int x1 = static_cast<int>(imgX);
                        int y1 = static_cast<int>(imgY);
                        int x2 = std::min(x1 + 1, m_brickWallPng.width() - 1);
                        int y2 = std::min(y1 + 1, m_brickWallPng.height() - 1);

                        float deltaX = imgX - x1;
                        float deltaY = imgY - y1;

                        QRgb pixel1 = m_brickWallPng.pixel(x1, y1);
                        QRgb pixel2 = m_brickWallPng.pixel(x2, y1);
                        QRgb pixel3 = m_brickWallPng.pixel(x1, y2);
                        QRgb pixel4 = m_brickWallPng.pixel(x2, y2);

                        QRgb interpolatedColor = qRgb(
                            static_cast<int>((1 - deltaY) * ((1 - deltaX) * qRed(pixel1) + deltaX * qRed(pixel2)) +
                                             deltaY * ((1 - deltaX) * qRed(pixel3) + deltaX * qRed(pixel4))),
                            static_cast<int>((1 - deltaY) * ((1 - deltaX) * qGreen(pixel1) + deltaX * qGreen(pixel2)) +
                                             deltaY * ((1 - deltaX) * qGreen(pixel3) + deltaX * qGreen(pixel4))),
                            static_cast<int>((1 - deltaY) * ((1 - deltaX) * qBlue(pixel1) + deltaX * qBlue(pixel2)) +
                                             deltaY * ((1 - deltaX) * qBlue(pixel3) + deltaX * qBlue(pixel4))));
                        if(x >= 0 && x < SCREEN::LOGICAL_SIZE.width() && y >= 0 && y < SCREEN::LOGICAL_SIZE.height())
                            image.setPixelColor(x, y, interpolatedColor);
                    }
                    else {
                        if(x >= 0 && x < SCREEN::LOGICAL_SIZE.width() && y >= 0 && y < SCREEN::LOGICAL_SIZE.height())
                            image.setPixelColor(x, y, Qt::black);
                    }
                }
                else // Floor
                {
                    image.setPixelColor(x, y, Qt::green);
                }
            }//for

        }//for

        // Update & Draw Objects
        for (auto &object : m_listObjects)
        {
            // Update Object Physics
            object->x += object->vx * m_deltaTime / 250;
            object->y += object->vy * m_deltaTime / 250;

            // Check if object is inside wall - set flag for removal
            if(object->x < 0 || object->y < 0) {
                object->bRemove = true;
                continue;
            }
            if (map[(int)object->x * nMapWidth + (int)object->y] == '#')
                object->bRemove = true;

            // Can object be seen?
            float fVecX = object->x - fPlayerX;
            float fVecY = object->y - fPlayerY;
            float fDistanceFromPlayer = sqrtf(fVecX*fVecX + fVecY*fVecY);

            float fEyeX = sinf(fPlayerA);
            float fEyeY = cosf(fPlayerA);

            // Calculate angle between lamp and players feet, and players looking direction
            // to determine if the lamp is in the players field of view
            float fObjectAngle = atan2f(fEyeY, fEyeX) - atan2f(fVecY, fVecX);
            if (fObjectAngle < -3.14159f)
                fObjectAngle += 2.0f * 3.14159f;
            if (fObjectAngle > 3.14159f)
                fObjectAngle -= 2.0f * 3.14159f;

            bool bInPlayerFOV = fabs(fObjectAngle) < fFOV / 2.0f;

            if (bInPlayerFOV && fDistanceFromPlayer >= 0.5f && fDistanceFromPlayer < fDepth && !object->bRemove)
            {
                float fObjectCeiling = (float)(SCREEN::LOGICAL_SIZE.height() / 2.0) - SCREEN::LOGICAL_SIZE.height() / ((float)fDistanceFromPlayer);
                float fObjectFloor = SCREEN::LOGICAL_SIZE.height() - fObjectCeiling;
                float fObjectHeight = fObjectFloor - fObjectCeiling;
                float fObjectAspectRatio = (float)object->sprite.height() / (float)object->sprite.width();
                float fObjectWidth = fObjectHeight / fObjectAspectRatio;
                float fMiddleOfObject = (0.5f * (fObjectAngle / (fFOV / 2.0f)) + 0.5f) * (float)SCREEN::LOGICAL_SIZE.width();

                // Draw Lamp
                for (float lx = 0; lx < fObjectWidth; lx++)
                {
                    for (float ly = 0; ly < fObjectHeight; ly++)
                    {
                        float fSampleX = lx / fObjectWidth;
                        float fSampleY = ly / fObjectHeight;
                        //wchar_t c = object.sprite->SampleGlyph(fSampleX, fSampleY);
                        int nObjectColumn = (int)(fMiddleOfObject + lx - (fObjectWidth / 2.0f));
                        if (nObjectColumn >= 0 && nObjectColumn < SCREEN::LOGICAL_SIZE.width() && (fObjectCeiling + ly) < SCREEN::LOGICAL_SIZE.height())
                        {
                            int imgX = fSampleX * (object->sprite.width() - 1);
                            int imgY = fSampleY * (object->sprite.height() - 1);
                            if (object->sprite.pixelColor(imgX, imgY).alpha() != 0 && fDepthBuffer[nObjectColumn] >= fDistanceFromPlayer)
                            {

                                //Draw(nObjectColumn, fObjectCeiling + ly, c, object.sprite->SampleColour(fSampleX, fSampleY));
                                if(nObjectColumn >= 0 && nObjectColumn < SCREEN::LOGICAL_SIZE.width() && fObjectCeiling + ly >= 0 && fObjectCeiling + ly < SCREEN::LOGICAL_SIZE.height())
                                    image.setPixelColor(nObjectColumn, fObjectCeiling + ly, object->sprite.pixelColor(imgX, imgY));
                                fDepthBuffer[nObjectColumn] = fDistanceFromPlayer;
                            }
                        }
                    }
                }
            }
        }

        // Remove dead objects from object list
        for (auto sobject : m_listObjects) {
            if(sobject->bRemove) {
                m_listObjects.removeOne(sobject);
            }
        }
        clear();
        QGraphicsPixmapItem* pItem = new QGraphicsPixmapItem(QPixmap::fromImage(image));
        pItem->setScale(4);
        pItem->setPos(0,0);
        addItem(pItem);
        drawMap();
        drawSilhouette();
        resetStatus();
    }//while
}

void GameScene::onUserCreate()
{
    setBackgroundBrush(COLOR_STYLE::BACKGROUND);
    for(int i = 0; i < 256; ++i)
    {
        m_keys[i] = new KeyStatus();
    }
    m_mouse = new MouseStatus();
    setSceneRect(0,0, SCREEN::PHYSICAL_SIZE.width(), SCREEN::PHYSICAL_SIZE.height());
    connect(&m_timer, &QTimer::timeout, this, &GameScene::loop);
    m_timer.start(int(1000.0f/FPS));
    m_elapsedTimer.start();

    map += QStringLiteral("#########.......#########.......");
    map += QStringLiteral("#...............#...............");
    map += QStringLiteral("#.......#########.......########");
    map += QStringLiteral("#..............##..............#");
    map += QStringLiteral("#......##......##......##......#");
    map += QStringLiteral("#......##..............##......#");
    map += QStringLiteral("#..............##..............#");
    map += QStringLiteral("###............####............#");
    map += QStringLiteral("##.............###.............#");
    map += QStringLiteral("#............####............###");
    map += QStringLiteral("#..............................#");
    map += QStringLiteral("#..............##..............#");
    map += QStringLiteral("#..............##..............#");
    map += QStringLiteral("#...........#####...........####");
    map += QStringLiteral("#..............................#");
    map += QStringLiteral("###..####....########....#######");
    map += QStringLiteral("####.####.......######..........");
    map += QStringLiteral("#...............#...............");
    map += QStringLiteral("#.......#########.......##..####");
    map += QStringLiteral("#..............##..............#");
    map += QStringLiteral("#......##......##.......#......#");
    map += QStringLiteral("#......##......##......##......#");
    map += QStringLiteral("#..............##..............#");
    map += QStringLiteral("###............####............#");
    map += QStringLiteral("##.............###.............#");
    map += QStringLiteral("#............####............###");
    map += QStringLiteral("#..............................#");
    map += QStringLiteral("#..............................#");
    map += QStringLiteral("#..............##..............#");
    map += QStringLiteral("#...........##..............####");
    map += QStringLiteral("#..............##..............#");
    map += QStringLiteral("################################");

    fDepthBuffer = new float[SCREEN::LOGICAL_SIZE.width()];
    m_listObjects.push_back(new sObject(8.5f, 8.5f, 0.0f, 0.0f, false, QImage(QString(":/res/sprites/lamp.png"))));
    m_listObjects.push_back(new sObject(7.5f, 7.5f, 0.0f, 0.0f, false, QImage(QString(":/res/sprites/lamp.png"))));
    m_listObjects.push_back(new sObject(10.5f, 3.5f, 0.0f, 0.0f, false, QImage(QString(":/res/sprites/lamp.png"))));
}

void GameScene::renderScene()
{
    static int index = 0;
    QString fileName = QDir::currentPath() + QDir::separator() + "screen" + QString::number(index++) + ".png";
    QRect rect = sceneRect().toAlignedRect();
    QImage image(rect.size(), QImage::Format_ARGB32);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    render(&painter);
    image.save(fileName);
    qDebug() << "saved " << fileName;
}

void GameScene::handlePlayerInput()
{
    if(m_keys[KEYBOARD::KeysMapper[Qt::Key_W]]->m_released)
    {
        fPlayerX += sinf(fPlayerA) * fSpeed;
        fPlayerY += cosf(fPlayerA) * fSpeed;
        if (map[(int)fPlayerX * nMapWidth + (int)fPlayerY] == '#')
        {
            fPlayerX -= sinf(fPlayerA) * fSpeed;
            fPlayerY -= cosf(fPlayerA) * fSpeed;
        }
    }

    if(m_keys[KEYBOARD::KeysMapper[Qt::Key_S]]->m_released)
    {
        fPlayerX -= sinf(fPlayerA) * fSpeed;
        fPlayerY -= cosf(fPlayerA) * fSpeed;
        if (map[(int)fPlayerX * nMapWidth + (int)fPlayerY] == '#')
        {
            fPlayerX += sinf(fPlayerA) * fSpeed;
            fPlayerY += cosf(fPlayerA) * fSpeed;
        }
    }

    if(m_keys[KEYBOARD::KeysMapper[Qt::Key_D]]->m_released)
    {
        fPlayerA += (fSpeed / 2.0f);
    }

    if(m_keys[KEYBOARD::KeysMapper[Qt::Key_A]]->m_released)
    {
        fPlayerA -= (fSpeed / 2.0f);
    }

    if(m_keys[KEYBOARD::KeysMapper[Qt::Key_E]]->m_released)
    {
        fPlayerX += cosf(fPlayerA) * fSpeed;
        fPlayerY -= sinf(fPlayerA) * fSpeed;
        if (map[(int)fPlayerX * nMapWidth + (int)fPlayerY] == '#')
        {
            fPlayerX -= cosf(fPlayerA) * fSpeed;
            fPlayerY += sinf(fPlayerA) * fSpeed;
        }
    }

    if(m_keys[KEYBOARD::KeysMapper[Qt::Key_Q]]->m_released)
    {
        fPlayerX -= cosf(fPlayerA) * fSpeed;
        fPlayerY += sinf(fPlayerA) * fSpeed;
        if (map[(int)fPlayerX * nMapWidth + (int)fPlayerY] == '#')
        {
            fPlayerX += cosf(fPlayerA) * fSpeed;
            fPlayerY -= sinf(fPlayerA) * fSpeed;
        }
    }

    if (m_keys[KEYBOARD::KeysMapper[Qt::Key_Space]]->m_released)
    {
        sObject* o = new sObject(fPlayerX, fPlayerY,
                                 sinf(fPlayerA) * 8,
                                 cosf(fPlayerA) * 8,
                                 false,
                                 QImage(QString(":/res/sprites/projectile.png")));
        m_listObjects.push_back(o);
    }

    if (m_keys[KEYBOARD::KeysMapper[Qt::Key_Z]]->m_released) {
        renderScene();
    }
}

void GameScene::resetStatus()
{
    for(int i = 0; i < 256; ++i)
    {
        m_keys[i]->m_released = false;
    }
    for(int i = 0; i < 256; ++i)
    {
        m_keys[i]->m_pressed = false;
    }
    m_mouse->m_released = false;
}

void GameScene::drawMap()
{
    QImage image = QImage(nMapWidth, nMapHeight, QImage::Format_RGB32);
    for(int x = 0; x < nMapWidth; ++x)
    {
        for(int y = 0; y < nMapHeight; ++y)
        {
            if(map[y * nMapWidth + x] == '#')
            {
                image.setPixelColor(x,y, QColor(Qt::red));
            }
            else
            {
                image.setPixelColor(x,y, QColor(Qt::yellow));
            }
        }
    }
    image.setPixelColor( (int)fPlayerY, (int)fPlayerX, QColor(Qt::green));
    QGraphicsPixmapItem* pItem = new QGraphicsPixmapItem(QPixmap::fromImage(image));
    pItem->setScale(2*4);
    pItem->setPos(0, 0);
    addItem(pItem);
}

void GameScene::drawSilhouette()
{
    QGraphicsPixmapItem* pItem = new QGraphicsPixmapItem(PixmapManager::Instance()->getPixmap(PixmapManager::TextureID::SILHOUETTE));
    pItem->setScale(1);
    pItem->setPos(SCREEN::PHYSICAL_SIZE.width() / 2 - pItem->boundingRect().width() / 4,
                  SCREEN::PHYSICAL_SIZE.height() - pItem->boundingRect().height());
    addItem(pItem);
}

void GameScene::keyPressEvent(QKeyEvent *event)
{
    if(KEYBOARD::KeysMapper.contains(event->key()))
    {
        if(event->isAutoRepeat())
        {
            m_keys[KEYBOARD::KeysMapper[event->key()]]->m_held = true;
            m_keys[KEYBOARD::KeysMapper[event->key()]]->m_pressed = false;
        }
        else
        {
            m_keys[KEYBOARD::KeysMapper[event->key()]]->m_pressed = true;
            m_keys[KEYBOARD::KeysMapper[event->key()]]->m_held    = false;
        }
    }
    QGraphicsScene::keyPressEvent(event);
}

void GameScene::keyReleaseEvent(QKeyEvent *event)
{
    if(KEYBOARD::KeysMapper.contains(event->key()))
    {
        if(!event->isAutoRepeat())
        {
            m_keys[KEYBOARD::KeysMapper[event->key()]]->m_held = false;
            m_keys[KEYBOARD::KeysMapper[event->key()]]->m_pressed = false;
            m_keys[KEYBOARD::KeysMapper[event->key()]]->m_released = true;
        }

    }
    QGraphicsScene::keyReleaseEvent(event);
}

void GameScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    m_mouse->m_x = event->scenePos().x();
    m_mouse->m_y = event->scenePos().y();
    m_mouse->m_pressed = true;
    QGraphicsScene::mousePressEvent(event);
}

void GameScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    m_mouse->m_x = event->scenePos().x();
    m_mouse->m_y = event->scenePos().y();
    QGraphicsScene::mouseMoveEvent(event);
}

void GameScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    m_mouse->m_x = event->scenePos().x();
    m_mouse->m_y = event->scenePos().y();
    m_mouse->m_pressed = false;
    m_mouse->m_released = true;
    QGraphicsScene::mouseReleaseEvent(event);
}
