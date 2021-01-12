// Generated by using Rcpp::compileAttributes() -> do not edit by hand
// Generator token: 10BE3573-1514-4C36-9D1C-5A225CD40393

#include <Rcpp.h>

using namespace Rcpp;

// blast
void blast(DataFrame query_table, DataFrame db_table, std::string output_file);
RcppExport SEXP _blaster_blast(SEXP query_tableSEXP, SEXP db_tableSEXP, SEXP output_fileSEXP) {
BEGIN_RCPP
    Rcpp::RNGScope rcpp_rngScope_gen;
    Rcpp::traits::input_parameter< DataFrame >::type query_table(query_tableSEXP);
    Rcpp::traits::input_parameter< DataFrame >::type db_table(db_tableSEXP);
    Rcpp::traits::input_parameter< std::string >::type output_file(output_fileSEXP);
    blast(query_table, db_table, output_file);
    return R_NilValue;
END_RCPP
}
// filter_fasta
DataFrame filter_fasta(std::string filename, std::string by, bool split);
RcppExport SEXP _blaster_filter_fasta(SEXP filenameSEXP, SEXP bySEXP, SEXP splitSEXP) {
BEGIN_RCPP
    Rcpp::RObject rcpp_result_gen;
    Rcpp::RNGScope rcpp_rngScope_gen;
    Rcpp::traits::input_parameter< std::string >::type filename(filenameSEXP);
    Rcpp::traits::input_parameter< std::string >::type by(bySEXP);
    Rcpp::traits::input_parameter< bool >::type split(splitSEXP);
    rcpp_result_gen = Rcpp::wrap(filter_fasta(filename, by, split));
    return rcpp_result_gen;
END_RCPP
}
// read_fasta
DataFrame read_fasta(std::string filename);
RcppExport SEXP _blaster_read_fasta(SEXP filenameSEXP) {
BEGIN_RCPP
    Rcpp::RObject rcpp_result_gen;
    Rcpp::RNGScope rcpp_rngScope_gen;
    Rcpp::traits::input_parameter< std::string >::type filename(filenameSEXP);
    rcpp_result_gen = Rcpp::wrap(read_fasta(filename));
    return rcpp_result_gen;
END_RCPP
}

static const R_CallMethodDef CallEntries[] = {
    {"_blaster_blast", (DL_FUNC) &_blaster_blast, 3},
    {"_blaster_filter_fasta", (DL_FUNC) &_blaster_filter_fasta, 3},
    {"_blaster_read_fasta", (DL_FUNC) &_blaster_read_fasta, 1},
    {NULL, NULL, 0}
};

RcppExport void R_init_blaster(DllInfo *dll) {
    R_registerRoutines(dll, NULL, CallEntries, NULL, NULL);
    R_useDynamicSymbols(dll, FALSE);
}
