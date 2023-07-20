#include <iostream>
#include <fstream>
#include <vector>

struct Element {
    int id;
    int first;
    int last;
    int x;
    int y;
    int w;
    int h;
};

struct Size {
    int width;
    int height;
};

void makeNewOrSetLast(std::vector<Element*>& elements, char a, char b, int offset)
{
    offset *= 2;
    Element* e = nullptr;
    
    for (int i = 0; i < elements.size(); i++) {
        if (elements[i]->id == a || elements[i]->id == b) {
            e = elements[i];
            break;
        }
    }

    if (e == nullptr) {
        e = new Element;
        e->id = (a != 0x00) ? a : b;
        e->first = 0;
        e->last = 0;
        e->x = 0;
        e->y = 0;
        e->w = 0;
        e->h = 0;

        if (a != 0x00) {
            e->first = offset - 2;
        }
        else {
            e->first = offset - 1;
        }

        elements.push_back(e);
    }

    if (b != 0x00) {
        e->last = offset - 1;
    }
    else {
        e->last = offset - 2;
    }

}

std::vector<Element*> tryGetElementsFromFile(std::fstream* file)
{
    std::vector<Element*> elements;
    uint32_t buff32 = NULL; uint8_t buff8 = NULL;
    
    // move coursor and get offset fo data
    file->seekg(0xA, std::ios_base::beg);
    file->read(reinterpret_cast<char*>(&buff32), sizeof(buff32));
    int dataOffset = buff32;

    // move for size of image
    file->seekg(0x12, std::ios_base::beg);

    // get width
    file->read(reinterpret_cast<char*>(&buff32), sizeof(buff32));
    int width = buff32;

    // get height
    file->read(reinterpret_cast<char*>(&buff32), sizeof(buff32));
    int height = buff32;

    // go to data offset
    file->seekg(dataOffset, std::ios_base::beg);

    char a, b;
    while (!file->eof()) {
        file->read(reinterpret_cast<char*>(&buff8), sizeof(buff8));
        a = (buff8 >> 4) & 0x0F;
        b = buff8 & 0x0F;
        if (a != 0x00 || b != 0x00) {
            makeNewOrSetLast(elements, a, b, (int)file->tellg()-dataOffset);
        }
    }
    file->clear();
    file->seekg(0x00, std::ios_base::beg);

    return elements;
}

Size getSize(std::fstream* file)
{
    uint32_t buff32 = NULL; Size size = Size();

    file->seekg(0x12, std::ios_base::beg);
    file->read(reinterpret_cast<char*>(&buff32), sizeof(buff32));
    size.width = buff32;
    file->read(reinterpret_cast<char*>(&buff32), sizeof(buff32));
    size.height = buff32;

    return size;
}

void setPositions(std::vector<Element*>& elements, int width, int height)
{
    Element* e = nullptr;

    for (int i = 0; i < elements.size(); i++) {
        e = elements[i];

        e->x = e->first % width;

        e->y = height - (e->first / width);

        e->w = e->last % width;
        e->w = e->w - e->x;
        e->w += 1;

        e->h = height - (e->last / width);
        e->h = e->y - e->h;
        e->h += 2;

        e->y -= e->h;
        e->y += (e->first % width > 0 ? 1 : 0);
    }
}

bool makeOutputFile(std::fstream* fileIn, std::fstream* fileOut)
{
    std::vector<Element*> elements = tryGetElementsFromFile(fileIn);
    if (elements.empty()) {
        printf("elements is empty\n");
        return false;
    }

    Size size = getSize(fileIn);
    printf("size of image: %ix%i\n", size.width, size.height);
    for (int i = 0; i < elements.size(); i++) {
        printf("id: 0x%X\tfirst: 0x%X\tlast: 0x%X\n", elements[i]->id, elements[i]->first, elements[i]->last);
    }
    printf("\n\n");

    setPositions(elements, size.width, size.height);
    for (int i = 0; i < elements.size(); i++) {
        printf("id: 0x%X = %i\n", elements[i]->id, elements[i]->id);
        printf("x: 0x%X = %i\n", elements[i]->x, elements[i]->x);
        printf("y: 0x%X = %i\n", elements[i]->y, elements[i]->y);
        printf("w: 0x%X = %i\n", elements[i]->w, elements[i]->w);
        printf("h: 0x%X = %i\n\n", elements[i]->h, elements[i]->h);
    }

    // make rrm file

    uint32_t buff;
    char signature[] = "rrmf";
    fileOut->write(reinterpret_cast<const char*>(&signature[0]), 4);
    
    buff = size.width;
    fileOut->write(reinterpret_cast<const char*>(&buff), sizeof(buff));

    buff = size.height;
    fileOut->write(reinterpret_cast<const char*>(&buff), sizeof(buff));

    buff = (uint32_t)elements.size();
    fileOut->write(reinterpret_cast<const char*>(&buff), sizeof(buff));

    for (int i = 0; i < elements.size(); i++) {
        buff = elements[i]->id;
        fileOut->write(reinterpret_cast<const char*>(&buff), sizeof(buff));

        buff = elements[i]->x;
        fileOut->write(reinterpret_cast<const char*>(&buff), sizeof(buff));

        buff = elements[i]->y;
        fileOut->write(reinterpret_cast<const char*>(&buff), sizeof(buff));

        buff = elements[i]->w;
        fileOut->write(reinterpret_cast<const char*>(&buff), sizeof(buff));

        buff = elements[i]->h;
        fileOut->write(reinterpret_cast<const char*>(&buff), sizeof(buff));
    }

    return true;
}

bool checkBmpFile(std::fstream* file)
{
    uint16_t format = NULL;
    file->read(reinterpret_cast<char*>(&format), sizeof(format));
    if (format != 0x4D42) {
        return false;
    }
    return true;
}

int main(int argc, char* argv[])
{
    if (argc < 2) {
        printf("usage: Bmp2RustyRoseMap \"filename\"\n");
        printf("Info: Work only with specyfic files made with paint.net [BMP, 4 bits]\n");
        return 0;
    }

    std::fstream fileIn;
    fileIn.open(argv[1], std::ios::in | std::ios::binary);
    
    if (!fileIn.good()) {
        printf("unable to open file\n");
        return 1;
    }

    std::fstream fileOut;
    fileOut.open(std::string(argv[1]) + ".rrm", std::ios::out | std::ios::binary);

    if (!checkBmpFile(&fileIn)) {
        printf("input file is in wrong format\n");
        return 2;
    }

    if (!makeOutputFile(&fileIn, &fileOut)) {
        printf("unable to make rusty rose map file\n");
        return 3;
    }

    fileIn.close();
    fileOut.close();
    return 0;
}