#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <string.h>


// This function encodes a given input message into an image file.
// The function accepts three parameters:
// input: a string representing the name of the input image file.
// txt: a string representing the message to be encoded.
// output: a string representing the name of the output image file.
// The function returns an integer:
// 0 if encoding was successful.
// 1 if any of the input arguments is NULL.
// 2 if the input image file cannot be opened.
// 3 if the message is too long to be encoded into the image.
// 4 if the output image file cannot be created.

union bit_set {
    unsigned char ch;
    struct {
        unsigned char bit0: 1;
        unsigned char bit1: 1;
        unsigned char bit2: 1;
        unsigned char bit3: 1;
        unsigned char bit4: 1;
        unsigned char bit5: 1;
        unsigned char bit6: 1;
        unsigned char bit7: 1;
    } bits;
    uint8_t byte;
};

int encode(const char *input, char *txt, const char *output)
{
// Check if any of the input arguments is NULL
    if(input == NULL || txt == NULL || output == NULL)
        return 1;

// Open the input image file
    FILE *fin = fopen(input, "r");
    if(fin == NULL)
        return 2;

// Create the output image file
    FILE *fout = fopen(output, "w");
    if(fout == NULL){
        fclose(fin);
        return 4;
    }

// Get the size of the input image file
    fseek(fin, 0, SEEK_END);
    long image_size = ftell(fin);
    fseek(fin, 0, SEEK_SET);

// Calculate the maximum size of the message that can be encoded into the image
    long unsigned int max_message_size = image_size / 8;

// Check if the message is too long to be encoded into the image
    if(strlen(txt) > max_message_size){
        fclose(fin);
        fclose(fout);
        return 3;
    }

// Create a buffer to store the bits of the message
    int size = 0;
    unsigned char bits[10000];

// Convert each character of the message into bits and store them in the buffer
    for(int i=0; i<(int)(strlen(txt)); i++){
        union bit_set bs;
        bs.ch = *(txt + i);
        *(bits + i*8) = bs.bits.bit7;
        *(bits + i*8 + 1) = bs.bits.bit6;
        *(bits + i*8 + 2) = bs.bits.bit5;
        *(bits + i*8 + 3) = bs.bits.bit4;
        *(bits + i*8 + 4) = bs.bits.bit3;
        *(bits + i*8 + 5) = bs.bits.bit2;
        *(bits + i*8 + 6) = bs.bits.bit1;
        *(bits + i*8 + 7) = bs.bits.bit0;
        size+=8;
    }

// Add a null terminator to the end of the bit buffer
    *(bits + size) = '\0';

// Iterate through each pixel of the image and modify its value to encode the message
    int pixel_value;
    int txt_current_len = 0;
    char c;
    int counter = 0;
    while(1){
        // Read the pixel value from the input file
        int res = fscanf(fin, "%d", &pixel_value);
        // If we've reached the end of the file, break out of the loop
        if(res == EOF)
            break;
        // If we couldn't read a pixel value, return an error
        if(res != 1) {
            fclose(fin);
            fclose(fout);
            return 3;
        }
    }

    // Return to the beginning of the input file
    fseek(fin, 0, SEEK_SET);
    const char *newline = "\n";

    // Loop through the message and encode it in the image
    while(txt_current_len < (int)max_message_size) {
        // Get the current position in the input file
        int current_pos = ftell(fin);
        int flag = 0;
        if(fscanf(fin, "%c", &c) == 1){
            // If the character is a newline, set the flag
            if(c == '\n')
                flag = 1;
        }

        c = (char)fgetc(fin);
        if (c == '\n') {
            fwrite(newline, sizeof(char), 1, fout);
            flag = 0;
        }

        ungetc(c, fin);
        // Return to the current position in the input file
        fseek(fin, current_pos, SEEK_SET);

        int res = fscanf(fin, "%d", &pixel_value);
        // If we've reached the end of the file, break out of the loop
        if (res == EOF)
            break;
        // If we couldn't read a pixel value, or the value is out of range, return an error
        if (res != 1 || pixel_value < 0 || pixel_value > 255) {
            fclose(fin);
            fclose(fout);
            return 3;
        }

        // Encode the message bit in the least significant bit of the pixel value
        if (txt_current_len < size){
            union bit_set bs_input;
            bs_input.byte = pixel_value;
            bs_input.bits.bit0 = *(bits + counter);
            counter++;
            char pixel_value_string[20];

            // Convert the pixel value to a string
            sprintf(pixel_value_string, "%d", bs_input.byte);

            // Write the encoded pixel value to the output file
            fprintf(fout, "%s", pixel_value_string);

            // Write a space after the pixel value, unless it's the last value
            if (txt_current_len < (int)max_message_size - 1) {
                fprintf(fout, " ");
            } else {
                // If it is the last value, write a newline instead
                fprintf(fout, "\n");
            }

            // If we've just written a newline, write another one
            if(flag){
                fprintf(fout, "\n");

            }

            // Increment the length of the encoded message
            txt_current_len++;
        }
        else {
            // If we've encoded the entire message, write the pixel value unchanged
            union bit_set bs_input;
            bs_input.byte = pixel_value;
            bs_input.bits.bit0 = 0;
            char pixel_value_string[20];

            // Convert the pixel value to a string
            sprintf(pixel_value_string, "%d", bs_input.byte);
            // Write the encoded pixel value to the output file
            fprintf(fout, "%s", pixel_value_string);

            if (txt_current_len < (int)max_message_size - 1) {
                fprintf(fout, " ");
            } else {
                fprintf(fout, "\n");
            }
        }

    }
    fclose(fin);
    fclose(fout);
    return 0;
}


// This function takes a filename, a pointer to a character array, and an integer size as input arguments.
// It tries to open the file with the given filename for reading, and if it fails, it returns 2.
// It reads each integer value from the file and uses its least significant bit to reconstruct the binary message.
// Once the binary message is constructed, it reads 8 bits at a time and converts them to characters.
// If any non-printable character (that is, not a letter, a digit, a punctuation mark or a white space) is encountered,
// the function returns 3. Finally, the function returns 0 to indicate success.
// The reconstructed message is stored in the character array pointed to by txt, which has a maximum size of size characters.

int decode(const char * filename, char *txt, int size)
{
    // Check for invalid input arguments
    if(filename == NULL || txt == NULL || size <= 0)
        return 1;

    // Try to open the file for reading
    FILE *fp = fopen(filename, "r");
    if(fp == NULL)
        return 2;

// Read the binary message from the file
    char binary_message[1000000];
    int pixel_value;
    int byte_count = 0;

    while(1)
    {
        int res = fscanf(fp, "%d", &pixel_value);
        if(res == EOF)
            break;
        if(res != 1) {
            fclose(fp);
            return 3;
        }
        // Extract the least significant bit from the pixel value and store it in the binary message
        union bit_set bs;
        bs.byte = pixel_value;
        *(binary_message + byte_count) = bs.bits.bit0 + '0';
        byte_count++;
    }

    fclose(fp);

// Null terminate the binary message string
    *(binary_message + byte_count) = '\0';

// Convert the binary message to text
    int len = 0;
    union bit_set bs;
    *txt = '\0';

// Round down the byte count to a multiple of 8
    byte_count = byte_count - (byte_count % 8);
    int flag = 0;
    for(int i=0; i<byte_count; i+=8){
        // Extract 8 bits at a time from the binary message and convert them to a character
        bs.bits.bit7 = *(binary_message + i);
        bs.bits.bit6 = *(binary_message + i + 1);
        bs.bits.bit5 = *(binary_message + i + 2);
        bs.bits.bit4 = *(binary_message + i + 3);
        bs.bits.bit3 = *(binary_message + i + 4);
        bs.bits.bit2 = *(binary_message + i + 5);
        bs.bits.bit1 = *(binary_message + i + 6);
        bs.bits.bit0 = *(binary_message + i + 7);

        if(len < size - 1){
            char ch = (char)bs.byte;
            *(txt + len) = ch;
            // Check if the character is printable
            if(!isalpha(ch) && !isspace(ch) && !isdigit(ch) && ch != 0 && !ispunct(ch)){
                flag = 1;
            }
            len++;
        }
    }
    *(txt + len) = '\0';

// Check if the message contains any non-printable characters
    if(flag)
        return 3;
    return 0;
}


int main() {
    char mode;
    char message[1001];
    char filename_input[30], filename_output[30];
    printf("Do you want to encode (E/e) or decode (D/d) a message?");
    if(scanf("%c", &mode) != 1){
        printf("Incorrect input");
        return 1;
    }
    while(getchar() != '\n'){}

    mode = (char)toupper(mode);
    if(mode != 'E' && mode != 'D'){
        printf("Incorrect input data");
        return 1;
    }

    switch(mode)
    {
        case 'D':

            printf("Enter input file name:");
            if(scanf("%29s", filename_input) != 1){
                printf("Incorrect input");
            }
            FILE *fin = fopen(filename_input, "r");
            if(fin == NULL){
                printf("couldn't open file");
                return 4;
            }

            fclose(fin);
            char txt[100000];
            int res = decode(filename_input, txt, sizeof(txt));
            if(res != 0){
                printf("file corrupted");
                return 6;
            }
            printf("%s", txt);
            break;

        case 'E':
            printf("Enter a message to be encoded:");
            if (fgets(message, sizeof(message), stdin) == NULL) {
                printf("Incorrect input");
                return 1;
            }
            char *newline = strchr(message, '\n');
            if (newline != NULL) {
                *newline = '\0';
            }
            printf("Enter input file name:");
            if(scanf("%29s", filename_input) != 1){
                printf("Incorrect input");
                return 1;
            }
            fin = fopen(filename_input, "r");

            printf("Enter output file name:");
            if(scanf("%29s", filename_output) != 1){
                printf("Incorrect input");
                fclose(fin);
                return 1;
            }
            if(fin == NULL){
                printf("couldn't open file");
                return 4;
            }
            FILE *fout = fopen(filename_output, "w");
            if(fout == NULL){
                fclose(fin);
                printf("couldn't create file");
                return 5;
            }
            res = encode(filename_input, message, filename_output);
            if(res != 0){
                fclose(fin);
                fclose(fout);
                printf("file corrupted");
                return 6;
            }
            printf("file saved");
            fclose(fin);
            fclose(fout);
            break;

        default:
            break; // condition is already checked
    }



    return 0;
}