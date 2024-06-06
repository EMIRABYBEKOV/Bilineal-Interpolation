#include <getopt.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <ctype.h>
#include <regex.h>
#include <errno.h>
#include "interpolate.h"

const int VERSIONS = 1;

const char *usage_msg = "Usage: %s <Eingabedatei> [options]\n"
"   -o S            Ausgabedatei\n"
"   -f N            Skalierungsfaktor\n"
"   or: %s -h       Eine Beschreibung aller Optionen des Programms.\n";

const char *help_msg =
"Positional arguments:\n"
"  <Dateiname>      Eingabedatei\n"
"Optional arguments:\n"
"  -V N             Welche Implementierung ausgeführt werden soll (default: N = 0 "
"(Hauptimplementierung))\n"
"  -B N             Messung der Laufzeit. Optionales Argument gibt die "
"Wiederholungen an. (default: N = 1)\n"
"  -o <Dateiname>   Ausgabedatei: S\n"
"  --coeffs a b c   Koeffizienten der Graustufenkonvertierung (a,b,c) Floating Point Zahlen\n"
"  -f N             Skalierungsfaktor\n"
"  -h | --help      Eine Beschreibung aller Optionen des Programms. (das hier)\n";

/**
 * @brief This function print_usage prints a text containing information on how
 * to use the program. Output is on stderr.
 * @param progname
 */
void print_usage(const char *progname) {
    fprintf(stderr, usage_msg, progname, progname);
}

/**
 * @brief This function print_help prints the usage information at first and
 * then the help text which contains information about what parameters are
 * allowed. Both texts are printed on stderr.
 * @param progname
 */
void print_help(const char *progname) {
    print_usage(progname);
    fprintf(stderr, "\n%s", help_msg);
}

/**
 * This function checks a string (char *) for values that are not digits
 * @param str string to be checked
 * @param progname name of the progname
*/
void is_digit(char *str, const char *progname) {
    for (size_t i = 0; i < strlen(str); i++) {
        if (!isdigit(str[i])) {
            fprintf(stderr, "Error: Argument ist keine Zahl.\n");
            print_usage(progname);
            exit(1);
        }
    }
}

/**
 * This function takes a file stream as one of it's params and "jumps" all whitespace characters
 * until the file position is at a non whitespace char.
 * @param stream file stream to be searched
 * @param progname name of the progname
*/
void jumpWhitespace(FILE *stream, const char *progname) {
    int a;
    while (isspace(a = fgetc(stream)) != 0);
    fseek(stream, -1, SEEK_CUR);

    // Check for '#'
    if (a == 35) {
        while (a != '\n') {
            a = fgetc(stream);
        }
        jumpWhitespace(stream, progname);
    }
}

/**
 * @brief This is the starting point of the program.
 * @param argc argument count
 * @param argv[] array of arguments
 */
int main(int argc, char *argv[]) {
    const char *progname = argv[0];

    // If the argument count is one, there's no argument passed since the name of
    // the program itself is argument one.
    if (argc == 1) {
        print_usage(progname);
        // Prettier for return 1 alias failure
        return EXIT_FAILURE;
    }

    // Positional argument
    FILE *instream = fopen(argv[1], "r");
    if (instream == NULL) {
        fprintf(stderr, "Error: Fehler beim Öffnen der Eingabedatei.\n");
        print_usage(progname);
        return EXIT_FAILURE;
    }

    // Declare variables
    size_t impl = 0;
    int outfd;
    char *outname;
    size_t scalFac = 0;
    float coeffs[3] = { 0, 0, 0 }; // If the user doesn't input anything default values will be set in grayscale
    size_t width, height;
    bool perf = false;
    size_t loops = 10; // Default value for how often the function should execute for performance testing
    
    // Regex to check for floats in coeffs
    regex_t rex;
    if (regcomp(&rex, "[[:digit:]+][.][[:digit:]+]", 0)) {
        fprintf(stderr, "Error: Fehler beim Kompilieren der Regex!\n");
        print_usage(progname);
        return EXIT_FAILURE;
    }

    int opt;
    // Declare long options which are options for the program but with 2 hyphen in
    // front. NOTE: it's only a forward to the short form
    static struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {"coeffs", required_argument, 0, 'c'},
        {0, 0, 0, 0}
    };
    // Check if the next optional argument is indeed on of the valid ones
    // x:  -> The parameter x must have a argument
    // x:: -> The parameter x may have a argument (optional argument)
    // x   -> The parameter x must have zero arguments
    while ((opt = getopt_long(argc, argv, "V:B::o:c:f:h", long_options, NULL)) !=
        -1) {
        switch (opt) {
            case 'V': // Implementation version
                is_digit(optarg, progname);
                impl = strtoul(optarg, NULL, 10);
                if (errno == ERANGE || impl > VERSIONS) {
                    fprintf(stderr, "Error: Das Argument 'V' muss zwischen 0 und %d sein.\n", VERSIONS);
                    print_usage(progname);
                    return EXIT_FAILURE;
                }
                break;
            case 'B': // Runtime measurement
                perf = true;
                if (optind < argc && *argv[optind] != '-') {
                    is_digit(argv[optind], progname);
                    loops = strtoul(argv[optind], NULL, 10);
                    if (errno == ERANGE || loops == 0 || loops >= INT32_MAX) {
                        fprintf(stderr, "Error: Die Performanzwiederholungen darf nicht 0 bzw. größer als INT_MAX sein.\n");
                        print_usage(progname);
                        return EXIT_FAILURE;
                    }
                }
                break;
            case 'o': // Output file
                outname = optarg;
                break;
            case 'c': // Coefficients
                ; // Fixed "A label can only be part of a statement"
                // A label cannot stand before a declaration
                // Therefore the statement ";" is set before
                char *fs = "";
                fs = strtok(optarg, ",");
                int i = 0;
                while (fs != NULL) {
                    // Check if fs is a float value
                    if (regexec(&rex, fs, 0, NULL, 0) != 0) {
                        fprintf(stderr, "Error: Mindestens ein Koeffizient ist kein Float. Vergiss nicht den Punkt als Kommazahl 3 -> 3.0\n");
                        print_usage(progname);
                        return EXIT_FAILURE;
                    }
                    coeffs[i++] = atof(fs);
                    fs = strtok(NULL, ",");
                }
                // Check if there are enough coeffs
                if (i != 3) {
                    fprintf(stderr, "Error: Nicht genügend Koeffizienten spezifiziert. %d von 3\n", i);
                    print_usage(progname);
                    return EXIT_FAILURE;
                }
                break;
            case 'f': // Scaling factor
                is_digit(optarg, progname);
                scalFac = strtoul(optarg, NULL, 10);
                if (errno == ERANGE || scalFac == 0) {
                    fprintf(stderr, "Error: Der Skalierungsfaktor darf nicht 0 bzw. größer als ULONG_MAX sein.\n");
                    print_usage(progname);
                    return EXIT_FAILURE;
                }
                break;
            case 'h': // Help
                print_help(progname);
                return EXIT_SUCCESS;
                break;
            default: // If there's a non-valid parameter passed, return a message on how
                // to use the program
                print_usage(progname);
                return EXIT_FAILURE;
                break;
        }
    }

    // Check for enough optional arguments
    if (scalFac == 0 || !outname) {
        fprintf(stderr, "Error: Skalierungsfaktor oder Ausgabedatei wurde nicht angegeben.\n");
        print_usage(progname);
        return EXIT_FAILURE;
    }

    // Open output file
    outfd = open(strcat(outname, ".pgm"), O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU);
    if (outfd < 0) {
        fprintf(stderr, "Error: Fehler beim erstellen der Ausgabedatei.\n");
        print_usage(progname);
        return EXIT_FAILURE;
    }

    // For ppm format reference look here: https://stackoverflow.com/questions/69581117/how-to-read-images-using-c
    // Check if reading file is working
    // Write the first 8 byte into buffer
    // These are meta data of the image
    // 1. Magic Number Test
    char buf[2];
    if (fread(buf, sizeof(char), sizeof(buf), instream) == 0) {
        fprintf(stderr, "Error: Lesen von der Eingabedatei hat nicht funktioniert.\n");
        print_usage(progname);
        return EXIT_FAILURE;
    }
    if (buf[0] != 'P' || buf[1] != '6') {
        fprintf(stderr, "Error: Falsche \"Magic Number\" der Eingabedatei.\n");
        print_usage(progname);
        return EXIT_FAILURE;
    }

    // 2. Whitespace
    jumpWhitespace(instream, progname);

    // 3. Width
    char buf2[100];
    int wcount = 0;
    while (isspace(buf2[wcount] = fgetc(instream)) == 0) {
        wcount++;
    }
    char widthStr[wcount];
    for (int i = 0; i < wcount; i++) {
        widthStr[i] = buf2[i];
    }
    width = strtoul(widthStr, NULL, 10);
    if (errno == ERANGE) {
        fprintf(stderr, "Error: Die Breite darf nicht größer als ULONG_MAX sein.\n");
        print_usage(progname);
        return EXIT_FAILURE;
    }
    fseek(instream, -1, SEEK_CUR);

    // 4. Whitespace
    jumpWhitespace(instream, progname);

    // 5. Height
    char buf3[100];
    int hcount = 0;
    while (isspace(buf3[hcount] = fgetc(instream)) == 0) {
        hcount++;
    }
    char heightStr[hcount];
    for (int i = 0; i < hcount; i++) {
        heightStr[i] = buf3[i];
    }
    height = strtoul(heightStr, NULL, 10);
    if (errno == ERANGE) {
        fprintf(stderr, "Error: Die Höhe darf nicht größer als ULONG_MAX sein.\n");
        print_usage(progname);
        return EXIT_FAILURE;
    }
    fseek(instream, -1, SEEK_CUR);

    // 6. Whitespace
    jumpWhitespace(instream, progname);

    // 7. Maxval Test
    char buf4[100];
    int mcount = 0;
    while (isspace(buf4[mcount] = fgetc(instream)) == 0) {
        mcount++;
    }
    char maxStr[mcount];
    for (int i = 0; i < mcount; i++) {
        maxStr[i] = buf4[i];
    }
    if (atol(maxStr) > 255) {
        fprintf(stderr, "Error: Der maximale Wert in der Eingabedatei ist größer 255 was keinem 24bpp Bild entspricht.\n");
        print_usage(progname);
        return EXIT_FAILURE;
    }
    fseek(instream, -1, SEEK_CUR);
    
    // 8. Whitespace
    if (isspace(fgetc(instream)) == 0) {
        fprintf(stderr, "Error: Whitespace nach dem maximalen Wert in der Eingabedatei existiert nicht.\n");
        print_usage(progname);
        return EXIT_FAILURE;
    }
    // printf("%ld %ld\n", width, height);

    // 9. Image data
    // Check for overflow
    if ((width != 0 && height > UINT64_MAX / width) || ((width * height) != 0 && 3 > UINT64_MAX / (width * height))) {
        fprintf(stderr, "Error: Länge des Eingabebildes generiert Overflow.\n");
        print_usage(progname);
        return EXIT_FAILURE;
    }
    size_t imglen = (width * height * 3);
    uint8_t *img = malloc(imglen);
    if(img == NULL) {
        fprintf(stderr, "Error: Speicherallokation für das Eingabebild hat nicht funktioniert.\n");
        print_usage(progname);
        return EXIT_FAILURE;
    }
    if (fread(img, sizeof(char), imglen, instream) == 0) {
        fprintf(stderr, "Error: Lesen von der Eingabedatei hat nicht funktioniert.\n");
        print_usage(progname);
        return EXIT_FAILURE;
    }
    // Check for whitespace in the beginning since there can only be 1 whitespace between maxval and rbgs
    if (isspace(img[0]) != 0) {
        fprintf(stderr, "Error: Es darf maximal nur ein Whitespace nach dem maximalen Wert in der Eingabedatei sein.\n");
        print_usage(progname);
        return EXIT_FAILURE;
    }

    // Close input file
    fclose(instream);

    // Create provisional tmp var
    // Check for overflow
    if ((width != 0 && scalFac > UINT64_MAX / width) || 
    (height != 0 && scalFac > UINT64_MAX / height) ||
    ((width * scalFac) != 0 && (height * scalFac) > UINT64_MAX / (width * scalFac))
    ) {
        fprintf(stderr, "Error: Länge des Ausgabebildes generiert Overflow.\n");
        print_usage(progname);
        return EXIT_FAILURE;
    }
    size_t reslen = (width * scalFac) * (height * scalFac);
    uint8_t *tmp = malloc(reslen);
    if(tmp == NULL) {
        fprintf(stderr, "Error: Speicherallokation für Zwischenergebnisse hat nicht funktioniert.\n");
        print_usage(progname);
        return EXIT_FAILURE;
    }

    // Create buffer for result
    uint8_t *result = malloc(reslen);
    if(result == NULL) {
        fprintf(stderr, "Error: Speicherallokation für das Ausgabebild hat nicht funktioniert.\n");
        print_usage(progname);
        return EXIT_FAILURE;
    }

    // Call function for interpolation
    double avgtime;
    if (perf) {
        // Performance testing is on
        struct timespec start;
        struct timespec end;
        switch (impl) {
            case 1:
                clock_gettime(1, &start); // 1 expands to CLOCK_MONOTONIC
                for (size_t i = 0; i < loops; ++i) {
                    interpolate_V1(img, width, height, coeffs[0], coeffs[1], coeffs[2], scalFac, tmp, result);
                }
                clock_gettime(1, &end);
                break;
            default: // case 0:
                clock_gettime(1, &start); // 1 expands to CLOCK_MONOTONIC
                for (size_t i = 0; i < loops; ++i) {
                    interpolate(img, width, height, coeffs[0], coeffs[1], coeffs[2], scalFac, tmp, result);
                }
                clock_gettime(1, &end);
                break;
        }
        double time = end.tv_sec - start.tv_sec + 1e-9 * (end.tv_nsec - start.tv_nsec);
        avgtime = time / loops;
    }
    else {
        switch (impl) {
            case 1:
                interpolate_V1(img, width, height, coeffs[0], coeffs[1], coeffs[2], scalFac, tmp, result);
                break;
            default: // case 0:
                interpolate(img, width, height, coeffs[0], coeffs[1], coeffs[2], scalFac, tmp, result);
                break;
        }
    }

    // Write result into output file
    // Get length of width and height
    size_t nw = width * scalFac;
    size_t nh = height * scalFac;
    short wl = 0;
    short hl = 0;
    while (nw > 0) {
        nw /= 10;
        wl++;
    }
    while (nh > 0) {
        nh /= 10;
        hl++;
    }
    char w[wl];
    char h[hl];

    // Funny comment to add
    char *fun = "# Emir, Lukas and Benji are cool!\n";
    char metadata[3 + strlen(fun) + wl + 1 + hl + 5];

    for (uint8_t i = 0; i < sizeof(metadata); i++) {
        metadata[i] = '\x00';
    }

    sprintf(w, "%lu", width * scalFac);
    sprintf(h, "%lu", height * scalFac);

    strcat(metadata, "P5\n");
    strcat(metadata, fun);
    strcat(metadata, w);
    strcat(metadata, " ");
    strcat(metadata, h);
    strcat(metadata, "\n255\n");

    if (write(outfd, metadata, sizeof(metadata)) < 0) {
        fprintf(stderr, "Error: Metadaten in die Ausgabedatei zu schreiben hat nicht funktioniert.\n");
        print_usage(progname);
        return EXIT_FAILURE;
    }

    // Format result data
    FILE *outstream = fdopen(outfd, "wb");
    fwrite(result, sizeof(char), (width * scalFac) * (height * scalFac), outstream);

    // Close output file stream
    fclose(outstream);
    close(outfd);

    // Free resources
    free(img);
    free(tmp);
    free(result);

    // Display metrics
    fprintf(stdout, "===========================================\n");
    fprintf(stdout, "Ergebnisse:\n");
    fprintf(stdout, "Version: %ld\n", impl);
    if (perf) {
        fprintf(stdout, "Performanz Wiederholungen: %lu\n", loops);
        fprintf(stdout, "Durschnittliche Laufzeit: %f Sekunden\n", avgtime);
    }
    fprintf(stdout, "Ausgabe in: %s\n", outname);
    fprintf(stdout, "===========================================\n");

    // Exit with success if everything worked fine
    return EXIT_SUCCESS;
}