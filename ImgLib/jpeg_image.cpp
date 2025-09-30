#include "ppm_image.h"

#include <array>
#include <fstream>
#include <stdio.h>
#include <setjmp.h>

#include <jpeglib.h>

using namespace std;

namespace img_lib {

// структура из примера LibJPEG
struct my_error_mgr {
    struct jpeg_error_mgr pub;
    jmp_buf setjmp_buffer;
};

typedef struct my_error_mgr* my_error_ptr;

// функция из примера LibJPEG
METHODDEF(void)
my_error_exit (j_common_ptr cinfo) {
    my_error_ptr myerr = (my_error_ptr) cinfo->err;
    (*cinfo->err->output_message) (cinfo);
    longjmp(myerr->setjmp_buffer, 1);
}

// В эту функцию вставлен код примера из библиотеки libjpeg.
// Измените его, чтобы адаптировать к переменным file и image.
// Задание качества уберите - будет использовано качество по умолчанию
bool SaveJPEG(const Path& file, const Image& image) {
    
    /* Эта структура содержит параметры сжатия JPEG и указатели на рабочее пространство (которое выделяется библиотекой JPEG по мере необходимости). Возможно, что одновременно существует несколько таких структур, представляющих несколько процессов сжатия/распаковки.  Мы называем любую структуру (и связанные с ней рабочие данные) "объектом JPEG". */
    struct jpeg_compress_struct cinfo;

    /* Эта структура представляет собой обработчик ошибок в формате JPEG.  Она объявлена отдельно, поскольку приложениям часто требуется предоставить специализированный обработчик ошибок (пример смотрите во второй половине этого файла).  Но здесь мы просто воспользуемся простым способом и воспользуемся стандартным обработчиком ошибок, который напечатает сообщение в stderr и вызовет exit() в случае сбоя сжатия. Обратите внимание, что эта структура должна работать так же долго, как и структура параметров main JPEG, чтобы избежать проблем с зависанием указателя.*/
    struct jpeg_error_mgr jerr;
    
    /* Еще кое-что */
    FILE * outfile;      /* целевой файл */
    JSAMPROW row_pointer[1];  /* указатель на JSAMPLE row[s] */
    int row_stride;       /* физическая ширина строки в буфере изображений */

    /* Шаг 1: выделите и инициализируйте объект сжатия JPEG */

    /* Сначала мы должны настроить обработчик ошибок, на случай если шаг инициализации потерпит неудачу. (Маловероятно, но это может произойти, если не хватает памяти.) Эта процедура заполняет содержимое структуры jerr и возвращает адрес jerr, который мы помещаем в поле ссылки в cinfo. */
    cinfo.err = jpeg_std_error(&jerr);

    /* Теперь мы можем инициализировать объект сжатия JPEG. */
    jpeg_create_compress(&cinfo);

    /* Шаг 2: укажите место назначения данных (например, файл) */ 
    /* Примечание: шаги 2 и 3 можно выполнять в любом порядке. */

    /* Здесь мы используем код, предоставленный библиотекой, для отправки сжатых данных в поток stdio. Вы также можете написать свой собственный код для выполнения других задач.
    ОЧЕНЬ ВАЖНО: используйте опцию "b" в fopen(), если вы находитесь на машине, которая требует этого для записи бинарных файлов. */
    if ((outfile = fopen(file.string().c_str(), "wb")) == NULL) {
    fprintf(stderr, "can't open %s\n", file.string().c_str());
    exit(1);
    return false;
    }
    jpeg_stdio_dest(&cinfo, outfile);

    /* Шаг 3: задаем параметры сжатия */

    /* Сначала мы предоставляем описание входного изображения. Необходимо заполнить четыре поля структуры cinfo: */
    cinfo.image_width = image.GetWidth();  /* ширина и высота изображения в пикселях */
    cinfo.image_height = image.GetHeight();
    cinfo.input_components = 3;       /* количество цветовых компонентов на пиксель */
    cinfo.in_color_space = JCS_RGB;  /* цветовое пространство входного изображения */
    /* Теперь используйте библиотечную процедуру для установки параметров сжатия по умолчанию.
    (Перед вызовом этого параметра необходимо задать как минимум cinfo.in_color_space, поскольку значения по умолчанию зависят от исходного цветового пространства.)
    */
    jpeg_set_defaults(&cinfo);

    /* Теперь вы можете установить любые параметры, отличные от значений по умолчанию, которые вы хотите.

    /* Step 4: Start compressor */

    /* Значение TRUE гарантирует, что мы создадим полноценный файл interchange-JPEG. Укажите значение TRUE, если вы не очень уверены в том, что делаете.*/
    jpeg_start_compress(&cinfo, TRUE);

    /* Step 5: while (остались строки сканирования, которые нужно записать) jpeg_write_scanlines(...); */

    /* Здесь мы используем библиотечную переменную состояния cinfo.next_scanline в качестве счетчика циклов, чтобы нам не приходилось отслеживать их самостоятельно. Чтобы упростить задачу, мы передаем одну строку сканирования за вызов; вы можете передавать больше, если хотите. */
    row_stride = image.GetWidth() * 3; /* JSAMPLEs для каждой строки в image_buffer */
    
    std::vector<JSAMPLE> buffer(row_stride);
    while (cinfo.next_scanline < cinfo.image_height) {
    /* "jpeg_write_scanlines ожидает массив указателей на scanlines. Здесь массив состоит только из одного элемента, но вы можете передать более одной scanline за раз, если это удобнее." */
        int y = cinfo.next_scanline;
        const Color* image_begin = image.GetLine(y);
        const Color* image_end = image_begin + image.GetWidth();
        int index = 0;
        for (;image_begin < image_end; ++image_begin) {
            buffer[index] = static_cast<JSAMPLE>(image_begin->r);
            buffer[index + 1] = static_cast<JSAMPLE>(image_begin->g);
            buffer[index + 2] = static_cast<JSAMPLE>(image_begin->b);
            index += 3;
        }
    row_pointer[0] = buffer.data();
    (void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }

    /* Шаг 6: Завершение сжатия */

    jpeg_finish_compress(&cinfo);
    /* После finish_compress мы можем закрыть выходной файл. */
    fclose(outfile);

    /* Шаг 7: освободите объект сжатия JPEG */

     /* Это важный шаг, так как он освободит большой объем памяти. */
    jpeg_destroy_compress(&cinfo);
    
    /* И мы закончили! */
    return true;
}

// тип JSAMPLE фактически псевдоним для unsigned char
void SaveSсanlineToImage(const JSAMPLE* row, int y, Image& out_image) {
    Color* line = out_image.GetLine(y);
    for (int x = 0; x < out_image.GetWidth(); ++x) {
        const JSAMPLE* pixel = row + x * 3;
        line[x] = Color{byte{pixel[0]}, byte{pixel[1]}, byte{pixel[2]}, byte{255}};
    }
}

Image LoadJPEG(const Path& file) {
    jpeg_decompress_struct cinfo;
    my_error_mgr jerr;
    
    FILE* infile;
    JSAMPARRAY buffer;
    int row_stride;

    // Тут не избежать функции открытия файла из языка C,
    // поэтому приходится использовать конвертацию пути к string.
    // Под Visual Studio это может быть опасно, и нужно применить
    // нестандартную функцию _wfopen
#ifdef _MSC_VER
    if ((infile = _wfopen(file.wstring().c_str(), "rb")) == NULL) {
#else
    if ((infile = fopen(file.string().c_str(), "rb")) == NULL) {
#endif
        return {};
    }

    /* Шаг 1: выделяем память и инициализируем объект декодирования JPEG */

    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = my_error_exit;

    if (setjmp(jerr.setjmp_buffer)) {
        jpeg_destroy_decompress(&cinfo);
        fclose(infile);
        return {};
    }

    jpeg_create_decompress(&cinfo);

    /* Шаг 2: устанавливаем источник данных */

    jpeg_stdio_src(&cinfo, infile);

    /* Шаг 3: читаем параметры изображения через jpeg_read_header() */

    (void) jpeg_read_header(&cinfo, TRUE);

    /* Шаг 4: устанавливаем параметры декодирования */

    // установим желаемый формат изображения
    cinfo.out_color_space = JCS_RGB;
    cinfo.output_components = 3;

    /* Шаг 5: начинаем декодирование */

    (void) jpeg_start_decompress(&cinfo);
    
    row_stride = cinfo.output_width * cinfo.output_components;
    
    buffer = (*cinfo.mem->alloc_sarray)
                ((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);

    /* Шаг 5a: выделим изображение ImgLib */
    Image result(cinfo.output_width, cinfo.output_height, Color::Black());

    /* Шаг 6: while (остаются строки изображения) */
    /*                     jpeg_read_scanlines(...); */

    while (cinfo.output_scanline < cinfo.output_height) {
        int y = cinfo.output_scanline;
        (void) jpeg_read_scanlines(&cinfo, buffer, 1);

        SaveSсanlineToImage(buffer[0], y, result);
    }

    /* Шаг 7: Останавливаем декодирование */

    (void) jpeg_finish_decompress(&cinfo);

    /* Шаг 8: Освобождаем объект декодирования */

    jpeg_destroy_decompress(&cinfo);
    fclose(infile);

    return result;
}

} // of namespace img_lib
