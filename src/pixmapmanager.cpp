#include "pixmapmanager.h"

PixmapManager* PixmapManager::ptr = nullptr;

PixmapManager *PixmapManager::Instance()
{
    if(!ptr)
    {
        ptr = new PixmapManager();
    }
    return ptr;
}

QPixmap& PixmapManager::getPixmap(TextureID id)
{
    return m_textures.get(id);
}

PixmapManager::PixmapManager()
{
    m_textures.load(TextureID::WALL, ":/res/sprites/wall64.png");
    m_textures.load(TextureID::SILHOUETTE, ":/res/sprites/silhouette.png");
    m_textures.load(TextureID::LAMP, ":/res/sprites/lamp.png");
    //std::unique_ptr<QPixmap> ButtonSelected(new QPixmap(getPixmap(TextureID::Buttons).copy(0,50,200,50)));
    //m_textures.insertResource(TextureID::ButtonSelected, std::move(ButtonSelected));
}
