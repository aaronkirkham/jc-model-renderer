#include <engine/formats/ExportedEntity.h>

ExportedEntity::ExportedEntity(const fs::path& file)
{
    // make sure this is an ee file
    if (file.extension() != ".ee") {
        throw std::invalid_argument("ExportedEntity type only supports .ee files!");
    }
}

ExportedEntity::~ExportedEntity()
{

}