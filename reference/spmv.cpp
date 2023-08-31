#include "../src/benchmark.hpp"
#include <cstdint>

namespace fs = std::filesystem;

template <typename T, typename I>
void experiment_spmv_csr(std::string input, std::string output, int verbose);

void experiment(std::string input, std::string output, int verbose){
    auto A_desc = json::parse(std::ifstream(fs::path(input)/"A"/"binsparse.json")); 
    auto x_desc = json::parse(std::ifstream(fs::path(input)/"x"/"binsparse.json")); 

    if (A_desc["format"] != "CSR") {throw std::runtime_error("Only CSR format for A is supported");}
    if (x_desc["format"] != "DVEC") {throw std::runtime_error("Only dense format for x is supported");}
    if (A_desc["data_types"]["pointers_to_1"] == "int32" &&
        A_desc["data_types"]["values"] == "float64") {
            experiment_spmv_csr<double, uint32_t>(input, output, verbose);
    } else if (A_desc["data_types"]["pointers_to_1"] == "int64" &&
        A_desc["data_types"]["values"] == "float64") {
            experiment_spmv_csr<double, uint64_t>(input, output, verbose);
    } else {
        throw std::runtime_error("Unsupported data types");
    }
}

template <typename T, typename I>
void experiment_spmv_csr(std::string input, std::string output, int verbose){
    auto A_desc = json::parse(std::ifstream(fs::path(input)/"A"/"binsparse.json")); 
    auto x_desc = json::parse(std::ifstream(fs::path(input)/"x"/"binsparse.json")); 

    int m = A_desc["shape"][0];
    int n = A_desc["shape"][1];

    auto x_val = npy_load_vector<T>(fs::path(input)/"x"/"values.npy");
    auto A_ptr = npy_load_vector<I>(fs::path(input)/"A"/"pointers_to_1.npy");
    auto A_ind = npy_load_vector<I>(fs::path(input)/"A"/"indices_1.npy");
    auto A_val = npy_load_vector<I>(fs::path(input)/"A"/"values.npy");

    auto y_val = std::vector<T>(m, 0);

    //perform an spmv of the matrix in c++

    for(int j = 0; j < m; j++){
        for(int p = A_ptr[j]; p < A_ptr[j+1]; p++){
            y_val[j] += A_val[p] * x_val[A_ind[p]];
        }
    }

    json y_desc;

    y_desc["version"] = 0.5;
    y_desc["format"] = "DVEC";
    y_desc["shape"] = {n};
    y_desc["nnz"] = n;
    y_desc["data_types"]["values"] = "float64";
    std::ofstream y_desc_file(fs::path(output)/"y"/"y.json");
    y_desc_file << y_desc;
    y_desc_file.close();

    npy_store_vector<T>(fs::path(output)/"y"/"values.npy", y_val);

    json measurements;
    measurements["time"] = 0;
    measurements["memory"] = 0;
    std::ofstream measurements_file(fs::path(output)/"measurements.json");
    measurements_file << measurements;
    measurements_file.close();
}