#include "seamcarving.h"
#include <stdio.h>
#include <math.h>

uint8_t comp_energy(struct rgb_img *im, int y, int x){
    // uint8_t pixel = get_pixel(im, x, y, 0); // Assuming you're accessing the red channel
    // printf("Pixel value: %d\n", pixel);

    uint8_t x_minus, x_plus, y_minus, y_plus;
    x_minus = (x - 1 + im->width) % im->width;
    x_plus = (x + 1) % im->width;
    y_minus = (y - 1 + im->height) % im->height;
    y_plus = (y + 1) % im->height;

    uint8_t Rx = abs(get_pixel(im, y, x_plus, 0) - get_pixel(im, y, x_minus, 0));
    uint8_t Gx = abs(get_pixel(im, y, x_plus, 1) - get_pixel(im, y, x_minus, 1));
    uint8_t Bx = abs(get_pixel(im, y, x_plus, 2) - get_pixel(im, y, x_minus, 2));

    uint8_t Ry = abs(get_pixel(im, y_plus, x, 0) - get_pixel(im, y_minus, x, 0));
    uint8_t Gy = abs(get_pixel(im, y_plus, x, 1) - get_pixel(im, y_minus, x, 1));
    uint8_t By = abs(get_pixel(im, y_plus, x, 2) - get_pixel(im, y_minus, x, 2));

    int delta_x_squared = Rx * Rx + Gx * Gx + Bx * Bx;
    int delta_y_squared = Ry * Ry + Gy * Gy + By * By;

    double energy = sqrt(delta_x_squared + delta_y_squared);

    uint8_t dual_gradient_energy = (uint8_t)(energy / 10);
    return dual_gradient_energy;
}

void calc_energy(struct rgb_img *im, struct rgb_img **grad){
    //turn pixel (color/direction) into value
    create_img(grad, im->height, im->width);
    
    int height = im->height;
    int width = im->width;

    for (int row = 0; row < height; row++){
        for (int col = 0; col < width; col++){
            // Ensure coordinates are within bounds
            uint8_t energy = comp_energy(im, row, col);
            (*grad)->raster[3 * ((row * width) + col) + 0] = energy;
        }
    }
}

double min_double(double a, double b, double c) {
    double min = b; // Assume 'a' is the minimum initially
    if (a < min) {
        min = b; // Update 'min' if 'b' is smaller
    }
    if (c < min) {
        min = c; // Update 'min' if 'c' is smaller
    }
    return min;
}

void dynamic_seam(struct rgb_img *grad, double **best_arr){
    size_t width = grad->width;
    size_t height = grad->height;

    (*best_arr) = (double *)malloc(height * width * sizeof(double));

    for (int j = 0; j < width; j++){
        (*best_arr)[j] = get_pixel(grad, 0, j, 0);
    }

    for (int i = 1; i < height; i++){
        for (int j = 0; j < width; j++){
            (*best_arr)[i * width + j] = grad->raster[3 * (i*(grad->width) + j)] + min_double((*best_arr)[(i-1) * width + j - 1], (*best_arr)[(i-1) * width + j], (*best_arr)[(i-1) * width + j + 1]);
        }
    }
}

int min_path(double a, double b, double c, int j) {
    double min = b; // Assume 'a' is the minimum initially
    int path = j;
    if (a < min) {
        min = a;
        path = j - 1; // Update 'min' if 'b' is smaller
    }
    if (c < min) {
        min = c;
        path = j + 1; // Update 'min' if 'c' is smaller
    }
    return path;
}

void recover_path(double *best, int height, int width, int **path){
    (*path) = (int *)malloc(height * sizeof(int));

    if (width > 0) {
        // Find the minimum value in the first row
        double min = best[0];
        int min_index = 0;
        for (int j = 1; j < width; j++) {
            if (best[j] < min) {
                min = best[j];
                min_index = j;
            }
        }
        (*path)[0] = min_index;

        for (int i = 1; i < height; i++) {
            int j = (*path)[i - 1]; // Previous column index
            (*path)[i] = min_path(best[(i) * width + j - 1], best[(i) * width + j], best[(i) * width + j + 1], j);
        }
    }
}

void remove_seam(struct rgb_img *src, struct rgb_img **dest, int *path){
    size_t new_width = src->width - 1;
    size_t height = src->height;
    size_t width = src->width;
    create_img(dest, height, new_width);

    for (int row = 0; row < height; row++){
        int dest_col = 0;
        for (int col = 0; col < width; col++){
            if (col == path[row]){
                continue;
            }
            dest_col++;
            int red = get_pixel(src, row, col, 0);
            int green = get_pixel(src, row, col, 1);
            int blue = get_pixel(src, row, col, 2);

            set_pixel((*dest), row, dest_col, red, green, blue);
        }
    }
    struct rgb_img *temp = *dest;  
    src = temp;  

    src->width = new_width;
}

int main(){
    // struct rgb_img *im;
    // struct rgb_img *cur_im;
    // struct rgb_img *grad;
    // double *best;
    // int *path;

    // read_in_img(&im, "HJoceanSmall.bin");
    
    // for(int i = 0; i < 5; i++){
    //     printf("i = %d\n", i);
    //     calc_energy(im,  &grad);
    //     dynamic_seam(grad, &best);
    //     recover_path(best, grad->height, grad->width, &path);
    //     remove_seam(im, &cur_im, path);

    //     char filename[200];
    //     sprintf(filename, "img%d.bin", i);
    //     write_img(cur_im, filename);


    //     destroy_image(im);
    //     destroy_image(grad);
    //     free(best);
    //     free(path);
    //     im = cur_im;
    // }
    // destroy_image(im);

    ////////////////////////////////////////////

    struct rgb_img *im;
    struct rgb_img *grad;
    
    char *file = "6x5.bin";
    read_in_img(&im, file);
    calc_energy(im, &grad);
    print_grad(grad);
    
    // Allocate memory for the best array
    size_t width = grad->width;
    size_t height = grad->height;
    double *best_arr;
    dynamic_seam(grad, &best_arr);
    
    // Print the best array
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            printf("%lf ", best_arr[i * width + j]);
        }
        printf("\n");
    }

    int *path;
    recover_path(best_arr, height, width, &path);
    printf("\n");
    for (int j = 0; j < height; j++){
        printf("%d ", (path)[j]);
    }
    printf("\n");

    struct rgb_img *dest;
    remove_seam(im, &dest, path);
    print_grad(dest);

    write_img(dest, "image2.bin");

    // Free memory allocated for grad and best_arr
    destroy_image(grad);
    free(best_arr);

    
}