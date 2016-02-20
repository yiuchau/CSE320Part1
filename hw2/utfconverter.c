#include "utfconverter.h"

#ifdef DEBUG
#define debug(fmt, ...) do{printf("DEBUG: %s:%s:%d " fmt, __FILE__,__FUNCTION__, __LINE__, ##__VA_ARGS__);}while(0)
#define info(fmt, ...) do{printf("INFO: %s:%s:%d " fmt, __FILE__,__FUNCTION__, __LINE__, ##__VA_ARGS__);}while(0)
#else
#define debug(fmt, ...)
#define info(fmt, ...) do{printf("INFO: " fmt,##__VA_ARGS__);}while(0)
#endif

#ifdef CSE320
#define  cse320(fmt, ...) do{printf("CSE320: %s:%s:%d " fmt, __FILE__,__FUNCTION__, __LINE__, ##__VA_ARGS__);}while(0)
#else
#define cse320(fmt, ...)
#endif


int main(int argc, char *argv[])
{
    int opt, vflag = 0, inputEncoding, outputEncoding, return_code = EXIT_FAILURE;
    char* hostName = malloc(16 * sizeof(char));
    char *evalue = NULL;
    char *input_path = NULL;
    char *output_path = NULL;


    /* Parse short options */
    while((opt = getopt(argc, argv, "hve:")) != -1) {
        switch(opt) {
            case 'h':
                fprintf(stderr, "The help menu was selected\n");
                USAGE(argv[0]);
                exit(EXIT_SUCCESS);
                break;
            case 'v':
                vflag++;
                break;
            case 'e':
                evalue = optarg;
                break;
            case '?':
                /* Let this case fall down to default;
                 * handled during bad option.
                 */
            default:
                fprintf(stderr,"A bad option was provided.\n");
                USAGE(argv[0]);
                exit(EXIT_FAILURE);
                break;
        }
    }

    if(evalue == NULL){
        fprintf(stderr,"Missing the '-e' flag. This flag is required. No output encoding format was provided\n");
        USAGE(argv[0]);
        exit(EXIT_FAILURE);
    }

    /* Check validity of output format */
    if(strcmp(evalue, "UTF-8") == 0){
        outputEncoding = 0;
    }else{
        if(strcmp(evalue, "UTF-16LE") == 0){
            outputEncoding = 1;
        }else{
            if(strcmp(evalue, "UTF-16BE") == 0){
                outputEncoding = 2;
            }else{
                fprintf(stderr,"Invalid output encoding format was provided: %s\n", evalue);
                USAGE(argv[0]);
                exit(EXIT_FAILURE);
            }
        }
    }

    debug("OutputEncoding: %s%d\n", evalue, outputEncoding);

    /* Get position arguments */
    if(optind < argc && (argc - optind) == 2) {
        input_path = argv[optind++];
        output_path = argv[optind++];
    } else {
        if((argc - optind) <= 0) {
            fprintf(stderr, "Missing INPUT_FILE and OUTPUT_FILE.\n");
        } else if((argc - optind) == 1) {
            fprintf(stderr, "Missing OUTPUT_FILE.\n");
        } else {
            fprintf(stderr, "Too many arguments provided.\n");
        }
        USAGE(argv[0]);
        exit(EXIT_FAILURE);
    }

    gethostname(hostName, 16);
    cse320("Host: %s\n", hostName);
    free(hostName);

    /* Make sure all the arguments were provided */
    if(input_path != NULL && output_path != NULL) {
        int input_fd = -1, output_fd = -1;
        unsigned char BOM_Value1, BOM_Value2, BOM_Value3;
        bool success = false;
        switch(validate_args(input_path, output_path)) {
                case VALID_ARGS:
                    /* Attempt to open the input file */
                    if((input_fd = open(input_path, O_RDONLY)) < 0) {
                        fprintf(stderr, "Failed to open the file %s\n", input_path);
                        perror(NULL);
                        goto conversion_done;
                    }
                    /* Delete the output file if it exists; Don't care about return code. */
                    unlink(output_path);
                    /* Attempt to create the file */
                    if((output_fd = open(output_path, O_CREAT | O_WRONLY,
                        S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)) < 0) {
                        /* Tell the user that the file failed to be created */
                        fprintf(stderr, "Failed to open the file %s\n", input_path);
                        perror(NULL);
                        goto conversion_done;
                    }

                    cse320("Output File: %s\n", output_path);

                    /* Read BOM and set input encoding */
                    read(input_fd, &BOM_Value1, 1);
                    read(input_fd, &BOM_Value2, 1);
                    if(BOM_Value1 == 0xFF && BOM_Value2 == 0xFE){
                        inputEncoding = 1;
                        cse320("Input Encoding: UTF-16LE\n");
                    }else{
                        if(BOM_Value1 == 0xFE && BOM_Value2 == 0xFF){
                            inputEncoding = 2;

                        cse320("Input Encoding: UTF-16BE\n");
                        }else{
                            read(input_fd, &BOM_Value3, 1);
                            if(BOM_Value1 == 0xEF && BOM_Value2 == 0xBB && BOM_Value3 == 0xBF){
                                inputEncoding = 0;

                        cse320("Input Encoding: UTF-8\n");
                            }else{
                                printf("Invalid BOM in input file");
                                USAGE(argv[0]);
                                exit(EXIT_FAILURE);
                            }
                        }
                    }

                    cse320("Output Encoding: %s\n", evalue);

                    
                    /* Write BOM of output encoding to output file */
                    outputEncoding_write(output_fd, outputEncoding);
                    cse320("The file %s was successfully created or modified.\n", output_path);
                    

                    /* Start the conversion */
                    success = convert(input_fd, output_fd, vflag, inputEncoding, outputEncoding);


conversion_done:
                        /* Conversion failed; clean up */
                    if(input_fd >= 0) {
                        close(input_fd);
                    }
                    if(output_fd >= 0) {
                        close(output_fd);
                    }

                    if(success) {
                        /* We got here so it must of worked right? */
                        return_code = EXIT_SUCCESS;
                    } else {
                        /* Just being pedantic... */
                        fprintf(stderr, "Error in conversion: EXIT_FAILURE.\n");
                        return_code = EXIT_FAILURE;
                    }
                    break;
                case SAME_FILE:
                    fprintf(stderr, "The output file %s was not created. Same as input file.\n", output_path);
                    break;
                case FILE_DNE:
                    fprintf(stderr, "The input file %s does not exist.\n", input_path);
                    break;
                default:
                    fprintf(stderr, "An unknown error occurred\n");
                    break;
        }
    } else {
        /* Alert the user what was not set before quitting. */
        if(input_path == NULL) {
            fprintf(stderr, "INPUT_FILE was not set.\n");
        }
        if(output_path == NULL) {
            fprintf(stderr, "OUTPUT_FILE was not set.\n");
        }
        // Print out the program usage
        USAGE(argv[0]);
    }

    return return_code;
}

int validate_args(const char *input_path, const char *output_path)
{
    int return_code = FAILED;
    /* Make sure both strings are not NULL */
    if(input_path != NULL && output_path != NULL) {
        /* Check to see if the the input and output are two different files. */
        if(strcmp(input_path, output_path) != 0) {
            /* Check to see if the input file exists */
            struct stat *sb = malloc(sizeof(struct stat));
            /* zero out the memory */
            memset(sb, 0, sizeof(struct stat));
            /* now check to see if the file exists */
            if(stat(input_path, sb) == -1) {
                /* something went wrong */
                if(errno == ENOENT) {
                    /* File does not exist. */
                    return_code = FILE_DNE;
                } else {
                    /* No idea what the error is. */
                    perror(NULL);
                }
            } else {

                cse320("Input File: %s, Inode Number: %d, Device Number: %d, File Size: %d\n", input_path, (int)(sb -> st_ino), (int)(sb -> st_dev), (int)(sb -> st_size));
                return_code = VALID_ARGS;
            }

            free(sb);  
                     
        } else {
            return_code = SAME_FILE;
        }
    }
    return return_code;
}

bool convert(const int input_fd, const int output_fd, int vflag, int inputEncoding, int outputEncoding){
    bool success = false;
    /* Convert from inputEncoding to outputEncoding */
    if(inputEncoding == 0){
        success = convert1(input_fd, output_fd, vflag, outputEncoding);
    }else{
        if((inputEncoding == 1 || inputEncoding == 2) && (outputEncoding == 2 || outputEncoding == 1)){
            /* Convert from UTF-16LE to UTF-16BE and vice-versa */
            success = convert2(input_fd, output_fd, vflag, inputEncoding, outputEncoding);
        }else{
            /* Convert from UTF-16LE and UTF-16BE to UTF-8 */
            success = convert3(input_fd, output_fd, vflag, inputEncoding, outputEncoding);
        }
    }

    return success;
}


bool convert2(const int input_fd, const int output_fd, int vflag, int inputEncoding, int outputEncoding)
{

    printf("convert2");
    /*
        Read in two bytes from input file, reverse the order and write to output file.
    */
    bool success = true;
    if(input_fd >= 0 && output_fd >= 0){
        uint8_t bytes[2];
        ssize_t bytes_read;


        printHeader(vflag);

        while(((bytes_read = read(input_fd, &bytes[0], 1)) == 1) &&
                ((bytes_read = read(input_fd, &bytes[1] , 1)) == 1)) {

            uint8_t buf[2];
            int codePoint;

            if(inputEncoding == outputEncoding){
                buf[0] = bytes[0];
                buf[1] = bytes[1];
            }else{
                buf[0] = bytes[1];
                buf[1] = bytes[0];
            }


            if((write(output_fd, buf, 2)) != 2) {
                /* The write operation failed */
                fprintf(stderr, "Write to file failed.\n");
                success = false;
            }

            if(inputEncoding == 1){
                codePoint = ((int)bytes[1] << 8 )+ (int)bytes[0];
            }else{
                codePoint = ((int)bytes[0] << 8) + (int)bytes[1];
            }

            debug("Vflag: %d\n", vflag);

            if(vflag >= 1){
                if(codePoint >= 32 && codePoint < 128){
                    printf("| %3c   ", codePoint);
                }else{
                    printf("| None  ");
                }
                printf("|%8d    ", 2);

                printf("|  U+%.4x ", codePoint);
            }

            if(vflag >= 2){
                printf("  |  0x%.2x%.2x", bytes[0], bytes[1]);
            }

            if(vflag >= 3){
                printf("  |  0x%.2x%.2x ", buf[0], buf[1]);
            }

            printDivider(vflag);

        }

        
    }else{
        success = false;
    }

    return success;
}

bool convert1(const int input_fd, const int output_fd, int vflag, int outputEncoding)
{
    bool success = false;
    if(input_fd >= 0 && output_fd >= 0) {
        /* UTF-8 encoded text can be @ most 4-bytes */
        unsigned char bytes[4];
        auto unsigned char read_value;
        auto size_t count = 0;
        auto ssize_t bytes_read;
        bool encode = false, parse_error = false;


        printHeader(vflag);
        /* Read in UTF-8 Bytes */
        while((bytes_read = read(input_fd, &read_value, 1)) == 1) {
            /* Mask the most significate bit of the byte */
            unsigned char masked_value = read_value & 0x80;
            if(masked_value == 0x80) {
                if((read_value & UTF8_4_BYTE) == UTF8_4_BYTE ||
                   (read_value & UTF8_3_BYTE) == UTF8_3_BYTE ||
                   (read_value & UTF8_2_BYTE) == UTF8_2_BYTE) {
                    // Check to see which byte we have encountered
                    if(count == 0) {
                        bytes[count++] = read_value;
                    } else {
                        /* Set the file position back 1 byte */
                        if(lseek(input_fd, -1, SEEK_CUR) < 0) {
                            /* failed to move the file pointer back */
                            parse_error = true;
                            perror(NULL);
                            fprintf(stderr, "Program has parse error\n");
                            goto conversion_done;
                        }
                        /* Enocde the current values into UTF-16LE */
                        encode = true;
                    }
                } else if((read_value & UTF8_CONT) == UTF8_CONT) {
                    /* continuation byte */
                    bytes[count++] = read_value;
                }
            } else {
                if(count == 0) {
                    /* US-ASCII */
                    bytes[count++] = read_value;
                    encode = true;
                } else {
                    /* Found an ASCII character but theres other characters
                     * in the buffer already.
                     * Set the file position back 1 byte.
                     */
                    if(lseek(input_fd, -1, SEEK_CUR) < 0) {
                        /* failed to move the file pointer back */
                        parse_error = true;
                        perror(NULL);
                        fprintf(stderr, "Program has parse error\n");
                        goto conversion_done;
                    }
                    /* Enocde the current values into UTF-16 */
                    encode = true;
                }
            }

            /* If its time to encode do it here */
            if(encode) {
                int i, value = 0;
                bool isAscii = false;
                for(i = 0; i < count; i++) {
                    debug("i is: %d, count is: %d\n", i, (int)count);
                    if(i == 0) {
                        if((bytes[i] & UTF8_4_BYTE) == UTF8_4_BYTE) {
                            value = bytes[i] & 0x7;
                        } else if((bytes[i] & UTF8_3_BYTE) == UTF8_3_BYTE) {
                            value =  bytes[i] & 0xF;
                        } else if((bytes[i] & UTF8_2_BYTE) == UTF8_2_BYTE) {
                            value =  bytes[i] & 0x1F;
                        } else if((bytes[i] & 0x80) == 0) {
                            /* Value is an ASCII character */
                            value = bytes[i];
                            isAscii = true;
                        } else {
                            /* Marker byte is incorrect */
                            parse_error = true;

                            fprintf(stderr, "Program has parse error\n");
                            goto conversion_done;
                        }
                    } else {
                        if(!isAscii) {
                            value = (value << 6) | (bytes[i] & 0x3F);
                            debug("The non-ascii code point is 0x%x\n", value);
                        } else {
                            /* How is there more bytes if we have an ascii char? */
                            parse_error = true;

                            fprintf(stderr, "Program has parse error\n");
                            goto conversion_done;
                        }
                    }
                }

                if(outputEncoding == 0){
                    ssize_t bytes_written;
                    if((bytes_written = (write(output_fd, bytes, count))) != count) {
                        /* The write operation failed */
                        fprintf(stderr, "Write to file failed. Expected %zu bytes but got %zd\n", count, bytes_written);
                        success = false;
                    }

                    verbosity(value, count, vflag, isAscii, bytes, outputEncoding);
                        
                }else if(value >= SURROGATE_PAIR){
                    /* Handle the value if its a surrogate pair*/
                    debug("Surrogate Pair\n");
                    int vprime; /* v` = v - 0x10000 */
                    vprime = value - SURROGATE_PAIR; /* subtract the constant from value */
                    int w1 = (vprime >> 10) + 0xD800;
                    int w2 =  (vprime & 0x3FF) + 0xDC00;
                    /* BE: Write surrogate pairs to file, w1 first */
                    if((!safe_write(input_fd, output_fd, &w1, CODE_UNIT_SIZE, outputEncoding)) ||
                        (!safe_write(input_fd, output_fd, &w2, CODE_UNIT_SIZE, outputEncoding))){

                            parse_error = true;
                            USAGE("Convert1");
                            fprintf(stderr, "Program has parse error\n");
                            goto conversion_done;
                        }

                        verbosity(w1, count, vflag, isAscii, bytes, outputEncoding);
                        verbosity(w2, count, vflag, isAscii, bytes, outputEncoding);
                    
                } else {
                    /* write the codeunit to file */
                    debug("Value being written: 0x%x\n", value);
                    if(!safe_write(input_fd, output_fd, &value, CODE_UNIT_SIZE, outputEncoding)) {
                        parse_error = true;
                        USAGE("Convert1");
                        fprintf(stderr, "Program has parse error\n");
                        goto conversion_done;
                    }

                    verbosity(value, count, vflag, isAscii, bytes, outputEncoding);
                }
                /* Done encoding the value to UTF-16LE */
                encode = false;
                count = 0;
            }
        }
        /* If we got here the operation was a success! */
        success = !parse_error;
    }
conversion_done:
    if(success){
        debug("Conversion returns success.\n");
    }else{
        debug("Conversion returns failure.\n");
    }
    return success;
}

void outputEncoding_write(const int output_fd, int outputEncoding){

    uint8_t BOM0[3];
    uint8_t BOM1[2];
    uint8_t BOM2[2];

    BOM0[0] = 0xEF; BOM0[1] = 0xBB; BOM0[2] = 0xBF;
    BOM1[0] = 0xFF; BOM1[1] = 0xFE;
    BOM2[0] = 0xFE; BOM2[1] = 0xFF;

    if(outputEncoding == 0){

        write(output_fd, BOM0, sizeof BOM0);
    }

    if(outputEncoding == 1){
        write(output_fd, BOM1, sizeof BOM1);
    }

    if(outputEncoding == 2){
        write(output_fd, BOM2, sizeof BOM2);
    }

    return;

}



bool safe_write(const int input_fd, const int output_fd, int *value, size_t size, int outputEncoding)
{
    bool success = true;
    ssize_t bytes_written; 
    /* int i; */

    uint32_t i = (uint32_t)*value;
    uint8_t buf[2];

    if(outputEncoding == 1){
        /* Order in LE format */
        buf[1] = (i & 0x0000ff00) >> 8;
        buf[0] = i & 0x000000ff;
    }else{
        if(outputEncoding == 2){
            /* Order in BE format */
        buf[0] = (i & 0x0000ff00) >> 8;
        buf[1] = i & 0x000000ff;
        }
    }

    /* Bitwise operations on value for endianess */

    if((bytes_written = write(output_fd, buf, sizeof buf)) != size) {
        /* The write operation failed */
        fprintf(stderr, "Write to file failed. Expected %zu bytes but got %zd\n", size, bytes_written);
        success = false;
    }

    return success;
}

void verbosity(int value, int count, int vflag, bool isAscii, unsigned char* bytes, int outputEncoding){

    uint32_t i = (uint32_t)value;
    uint8_t buf[2];

    if(outputEncoding == 1){
        /* Order in LE format */
        buf[1] = (i & 0x0000ff00) >> 8;
        buf[0] = i & 0x000000ff;
    }else{
        if(outputEncoding == 2){
            /* Order in BE format */
        buf[0] = (i & 0x0000ff00) >> 8;
        buf[1] = i & 0x000000ff;
        }
    }

    debug("Vflag: %d   ", vflag);

    if(vflag >= 1){

    if(value >= 32 && value <= 128){
        printf("| %3c   ", value);
    }else{
        printf("| None  ");
    }

    printf("|%8d    ", count);

    printf("|  U+%.4x ", value);
    }

    if(vflag >= 2){

    int i = 0;
    printf("  |  0x");

    while(i  < count){
        printf("%02x", bytes[i]);
        i++;
    }


    }

    if(vflag >= 3){
        if(outputEncoding == 0){
            printf("  |  0x");
            for(i = 0; i < count; i++){
                printf("%02x",bytes[i]);
            }
        }else{
            printf("  |  0x%02x%02x ", buf[0], buf[1]);
        }
    }

    printDivider(vflag);

    return;
}


bool convert3(const int input_fd, const int output_fd, int vflag, int inputEncoding, int outputEncoding)
{
    /*
        Read in two bytes from input file, reverse the order and write to output file.
    */
    bool success = true;
    if(input_fd >= 0 && output_fd >= 0){
        uint8_t bytes[4], outputBytes[4];
        ssize_t bytes_read;
        int count;
        int outputCount;
        int i;

        printHeader(vflag);

        while(((bytes_read = read(input_fd, &bytes[0], 1)) == 1) &&
                ((bytes_read = read(input_fd, &bytes[1] , 1)) == 1)) {
            count = 2;
            int codePoint;

            if(inputEncoding == 1){
                codePoint = (bytes[1] << 8) + bytes[0]; 
                if((codePoint & 0xD800) == 0xD800){
                    if(((bytes_read = read(input_fd, &bytes[3], 1)) == 1) &&
                        ((bytes_read = read(input_fd, &bytes[4] , 1)) == 1)){
                        int i = (bytes[4] << 8) + bytes[3];
                        if((i & 0xDC00) == 0xDC00){
                            codePoint = ((codePoint - 0xD800)  << 10) + (i - 0xDC00) + 0x10000;
                            count = 4;
                        } 
                    }

                }
            }
            else{
                codePoint = (bytes[0] << 8) + bytes[1];
                if((codePoint & 0xD800) == 0xD800){
                    if(((bytes_read = read(input_fd, &bytes[3], 1)) == 1) &&
                        ((bytes_read = read(input_fd, &bytes[4] , 1)) == 1)){
                        int i = (bytes[3] << 8) + bytes[4];
                        if((i & 0xDC00) == 0xDC00){
                            codePoint = ((codePoint - 0xD800)  << 10) + (i - 0xDC00) + 0x10000;
                            count = 4;
                        } 
                    }

                }
            }

            if(codePoint >= 0x0 && codePoint<= 0x7F){
                /* UTF-8 is one byte */
                outputBytes[0] = codePoint;
                outputCount = 1;
            }else if(codePoint >= 0x80 && codePoint <= 0x7FF){
                /* UTF-8 is two bytes */
                outputBytes[0] = UTF8_2_BYTE | (codePoint >> 6);
                outputBytes[1] = UTF8_CONT | (codePoint & 0x3F);
                outputCount = 2;
            }else if(codePoint >= 0x800 && codePoint <= 0xFFFF){
                /* UTF-8 is three bytes */
                outputBytes[0] = UTF8_3_BYTE | (codePoint >> 12);
                outputBytes[1] = UTF8_CONT | ((codePoint >> 6) & 0x3f);
                outputBytes[2] = UTF8_CONT | (codePoint & 0x3F);
                outputCount = 3;
            }else{
                /* UTF-8 is four bytes */
                outputBytes[0] = UTF8_4_BYTE | (codePoint >> 18);
                outputBytes[1] = UTF8_CONT | ((codePoint >> 12) & 0x3f);
                outputBytes[2] = UTF8_CONT | ((codePoint >> 6) & 0x3f);
                outputBytes[3] = UTF8_CONT | (codePoint & 0x3F);
                outputCount = 4;
            }


            if((write(output_fd, outputBytes, outputCount) != outputCount)) {
                /* The write operation failed */
                fprintf(stderr, "Write to file failed.\n");
                success = false;
            }

            debug("Vflag: %d\n", vflag);


            if(vflag >= 1){
                if(codePoint >= 32 && codePoint < 128){
                    printf("| %3c   ", codePoint);
                }else{
                    printf("| None  ");
                }
                printf("|%8d    ", count);

                printf("|  U+%.4x ", codePoint);
            }

            if(vflag >= 2){
                printf("  |  0x");
                for(i = 0; i < count; i++){
                    printf("%.2x",bytes[i]);
                }
            }

            if(vflag >= 3){
                printf("  |  0x");
                for(i = 0; i < outputCount; i++){
                    printf("%.2x",outputBytes[i]);
                }
                printf("  ");
            }

            printDivider(vflag);

        }

        
    }else{
        success = false;
    }

    return success;
}

void printHeader(int vflag){
    if(vflag == 1){
        printf(
            "+-------+------------+-----------+\n"
            "| ASCII | # of bytes | codepoint |\n"
            "+-------+------------+-----------+\n");
    }

    if(vflag == 2){
        printf(
            "+-------+------------+-----------+---------+\n"
            "| ASCII | # of bytes | codepoint |  input  |\n"
            "+-------+------------+-----------+---------+\n");
    }

    if(vflag >= 3){
        printf(
            "+-------+------------+-----------+---------+---------+\n"
            "| ASCII | # of bytes | codepoint |  input  |  output \n"
            "+-------+------------+-----------+---------+---------+\n");
    }
}

void printDivider(int vflag){
    if(vflag == 1){
        printf("|\n+-------+------------+-----------+\n");
    }

    if(vflag == 2){
        printf("|\n+-------+------------+-----------+---------+\n");
    }

    if(vflag >= 3){
        printf("|\n+-------+------------+-----------+---------+---------+\n");
    }
}