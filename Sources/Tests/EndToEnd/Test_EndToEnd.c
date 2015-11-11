#include "Extraction/Extract.h"
#include "General/pgm.h"
#include "Templates/Template.h"
#include "unity.h"
#include "unity_fixture.h"
#include <assert.h>
#include <string.h>

TEST_GROUP(EndToEnd);

static const char *inputFiles[] =
{
    "../TestImages/Person1/Bas1440999265-Hamster-0-0.png.pgm",
    "../TestImages/Person1/Bas1440999265-Hamster-0-1.png.pgm",
    "../TestImages/Person1/Bas1440999265-Hamster-0-2.png.pgm",
    "../TestImages/Person1/Bas1440999265-Hamster-1-0.png.pgm",
    "../TestImages/Person1/Bas1440999265-Hamster-1-1.png.pgm",
    "../TestImages/Person1/Bas1440999265-Hamster-1-2.png.pgm"
};

TEST_SETUP(EndToEnd)
{
}

TEST_TEAR_DOWN(EndToEnd)
{
}

static void ReadTemplate(const char *expectedFileName, Template *expectedTemplate)
{
    FILE *f = fopen(expectedFileName, "rb");
    
    int ret = fread(&expectedTemplate->originalDpi, sizeof(int32_t), 1, f);
    TEST_ASSERT_TRUE_MESSAGE(ret == 1, "ReadTemplate: failed originalDPI");
    
    ret = fread(&expectedTemplate->originalHeight, sizeof(int32_t), 1, f);
    TEST_ASSERT_TRUE_MESSAGE(ret == 1, "ReadTemplate: failed originalHeight");
    
    ret = fread(&expectedTemplate->originalWidth, sizeof(int32_t), 1, f);
    TEST_ASSERT_TRUE_MESSAGE(ret == 1, "ReadTemplate: failed originalWidth");
    
    int32_t count;
    ret = fread(&count, sizeof(int32_t), 1, f);
    TEST_ASSERT_TRUE_MESSAGE(ret == 1, "ReadTemplate: failed on minutiae count");
    
    for(int i = 0; i < count; i++) 
    {
        TemplateMinutia *minutia = calloc(1, sizeof(TemplateMinutia));
        
        ret = fread(&(minutia->direction), sizeof(int8_t), 1, f);
        TEST_ASSERT_TRUE_MESSAGE(ret == 1, "ReadTemplate: failed on minutia->direction");
        
        ret = fread(&(minutia->position.x), sizeof(int32_t), 1, f);
        TEST_ASSERT_TRUE_MESSAGE(ret == 1, "ReadTemplate: failed minutia->position.x");
        
        ret = fread(&(minutia->position.y), sizeof(int32_t), 1, f);
        TEST_ASSERT_TRUE_MESSAGE(ret == 1, "ReadTemplate: failed minutia->position.y");
        
        ret = fread(&(minutia->type), sizeof(int32_t), 1, f);
        TEST_ASSERT_TRUE_MESSAGE(ret == 1, "ReadTemplate: failed on minutia->type");
        
        Template_AddMinuitia(expectedTemplate, minutia); 
    }
    
    /* Check end of file */
    uint8_t tmp;
    ret = fread(&tmp, sizeof(uint8_t), 1, f);
    TEST_ASSERT_TRUE_MESSAGE(ret == 0 && feof(f), "ReadTemplate: Bad end of file");
 
    /* Close file */
    ret = fclose(f);
    assert(ret != EOF);
}


static void UnityFreeTemplate(Template *template)
{
    while (List_GetCount(&template->minutiae) > 0)
    {
        void *dataFound;
        List_Remove(&template->minutiae, template->minutiae.tail, &dataFound);
        free(dataFound);
    }
}

static void ImageToTemplate(const char *inputFileName, const char *expectedFileName)
{
    UInt8Array2D image = pgm_read(inputFileName);

    TEST_ASSERT_TRUE_MESSAGE(image.sizeX > 1, "Image Size X < 1");
    TEST_ASSERT_TRUE_MESSAGE(image.sizeY > 1, "Image Size Y < 1");

    struct perfdata perfdata;

    Template template = Template_Constuct();    
    Template expectedTemplate = Template_Constuct();

    printf("%s %s\r\n", inputFileName, expectedFileName);

    Extract(&image, &template, &perfdata, NULL, NULL);

    ReadTemplate(expectedFileName, &expectedTemplate);
    
    printf("originalDpi    %d %d\n", expectedTemplate.originalDpi, template.originalDpi);
    printf("originalWidth  %d %d\n", expectedTemplate.originalWidth, template.originalWidth);
    printf("originalHeight %d %d\n", expectedTemplate.originalHeight, template.originalHeight);
    printf("#minutiae      %d %d\n", expectedTemplate.minutiae.count, template.minutiae.count);
    List matching = List_Construct();
    List missingFromExpected = List_Construct();
    List missingFromTemplate= List_Construct();
    
    for (ListElement *i = expectedTemplate.minutiae.head; i != NULL; i = i->next)
    {
        TemplateMinutia *expected = i->data;
        bool found = false;
        for (ListElement *j = template.minutiae.head; j != NULL; j = j->next)
        {
            TemplateMinutia *templateMin = i->data;
            if ((templateMin->type == expected->type) && (templateMin->direction == expected->direction) && (templateMin->position.x == expected->position.x) && (templateMin->position.y == expected->position.y))
            {
                List_AddData(&matching, expected);
                found = true;
            }
        }
        if (!found)
        {
            List_AddData(&missingFromTemplate, expected);
        }
    }
    if (expectedTemplate.minutiae.count > 0)
    {
        printf("Matching = %d%%\n", matching.count * 100 / expectedTemplate.minutiae.count);
        printf("Missing from template = %d%%\n", missingFromTemplate.count * 100 / expectedTemplate.minutiae.count);        
        printf("Missing from expected = %d%%\n", missingFromExpected.count * 100 / expectedTemplate.minutiae.count);
    }

    
    UnityFreeTemplate(&template);
    UnityFreeTemplate(&expectedTemplate);
}

TEST(EndToEnd, ImageToTemplate)
{
    int length = sizeof(inputFiles) / sizeof(inputFiles[0]);
    for(int i = 0; i < length; i++) 
    {
        char templateFile[256];
        strcpy(templateFile, inputFiles[i]);
        strcat(templateFile, ".template");
        ImageToTemplate(inputFiles[i], templateFile);
    }
}
