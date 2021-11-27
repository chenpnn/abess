// #define R_BUILD
#ifdef R_BUILD

#include <Rcpp.h>
#include <RcppEigen.h>
//[[Rcpp::depends(RcppEigen)]]
using namespace Rcpp;

#else

#include <Eigen/Eigen>
#include "List.h"

#endif

#include <iostream>
#include "Data.h"
#include "Algorithm.h"
#include "AlgorithmPCA.h"
#include "AlgorithmGLM.h"
#include "Metric.h"
#include "path.h"
#include "utilities.h"
#include "abess.h"
#include "screening.h"
#include <vector>

typedef Eigen::Triplet<double> triplet;

#ifdef _OPENMP
#include <omp.h>
// [[Rcpp::plugins(openmp)]]
#else
#ifndef DISABLE_OPENMP
// use pragma message instead of warning
#pragma message("Warning: OpenMP is not available, "                 \
                "project will be compiled into single-thread code. " \
                "Use OpenMP-enabled compiler to get benefit of multi-threading.")
#endif
inline int omp_get_thread_num()
{
  return 0;
}
inline int omp_get_num_threads() { return 1; }
inline int omp_get_num_procs() { return 1; }
inline void omp_set_num_threads(int nthread) {}
inline void omp_set_dynamic(int flag) {}
#endif

using namespace Eigen;
using namespace std;

// [[Rcpp::export]]
List abessGLM_API(Eigen::MatrixXd x, Eigen::MatrixXd y, int n, int p, int normalize_type,
               Eigen::VectorXd weight, 
               int algorithm_type, int model_type, int max_iter, int exchange_num,
               int path_type, bool is_warm_start,
               int ic_type, double ic_coef, int Kfold,
               Eigen::VectorXi sequence,
               Eigen::VectorXd lambda_seq,
               int s_min, int s_max, 
               double lambda_min, double lambda_max, int nlambda,
               int screening_size, 
               Eigen::VectorXi g_index,
               Eigen::VectorXi always_select,
               int primary_model_fit_max_iter, double primary_model_fit_epsilon,
               bool early_stop, bool approximate_Newton,
               int thread,
               bool covariance_update,
               bool sparse_matrix,
               int splicing_type,
               int sub_search,
               Eigen::VectorXi cv_fold_id)
{
#ifdef _OPENMP
  // Eigen::initParallel();
  int max_thread = omp_get_max_threads();
  if (thread == 0 || thread > max_thread)
  {
    thread = max_thread;
  }

  Eigen::setNbThreads(thread);
  omp_set_num_threads(thread);

#endif
  int algorithm_list_size = max(thread, Kfold);
  vector<Algorithm<Eigen::VectorXd, Eigen::VectorXd, double, Eigen::MatrixXd> *> algorithm_list_uni_dense(algorithm_list_size);
  vector<Algorithm<Eigen::MatrixXd, Eigen::MatrixXd, Eigen::VectorXd, Eigen::MatrixXd> *> algorithm_list_mul_dense(algorithm_list_size);
  vector<Algorithm<Eigen::VectorXd, Eigen::VectorXd, double, Eigen::SparseMatrix<double>> *> algorithm_list_uni_sparse(algorithm_list_size);
  vector<Algorithm<Eigen::MatrixXd, Eigen::MatrixXd, Eigen::VectorXd, Eigen::SparseMatrix<double>> *> algorithm_list_mul_sparse(algorithm_list_size);

  for (int i = 0; i < algorithm_list_size; i++)
  {
    if (!sparse_matrix)
    {
      if (model_type == 1)
      {
        abessLm<Eigen::MatrixXd> *temp = new abessLm<Eigen::MatrixXd>(algorithm_type, model_type, max_iter, primary_model_fit_max_iter, primary_model_fit_epsilon, is_warm_start, exchange_num, always_select, splicing_type, sub_search);
        temp->covariance_update = covariance_update;
        algorithm_list_uni_dense[i] = temp;
      }
      else if (model_type == 2)
      {
        abessLogistic<Eigen::MatrixXd> *temp = new abessLogistic<Eigen::MatrixXd>(algorithm_type, model_type, max_iter, primary_model_fit_max_iter, primary_model_fit_epsilon, is_warm_start, exchange_num, always_select, splicing_type, sub_search);
        temp->approximate_Newton = approximate_Newton;
        algorithm_list_uni_dense[i] = temp;
      }
      else if (model_type == 3)
      {
        abessPoisson<Eigen::MatrixXd> *temp = new abessPoisson<Eigen::MatrixXd>(algorithm_type, model_type, max_iter, primary_model_fit_max_iter, primary_model_fit_epsilon, is_warm_start, exchange_num, always_select, splicing_type, sub_search);
        temp->approximate_Newton = approximate_Newton;
        algorithm_list_uni_dense[i] = temp;
      }
      else if (model_type == 4)
      {
        abessCox<Eigen::MatrixXd> *temp = new abessCox<Eigen::MatrixXd>(algorithm_type, model_type, max_iter, primary_model_fit_max_iter, primary_model_fit_epsilon, is_warm_start, exchange_num, always_select, splicing_type, sub_search);
        temp->approximate_Newton = approximate_Newton;
        algorithm_list_uni_dense[i] = temp;
      }
      else if (model_type == 5)
      {
        abessMLm<Eigen::MatrixXd> *temp = new abessMLm<Eigen::MatrixXd>(algorithm_type, model_type, max_iter, primary_model_fit_max_iter, primary_model_fit_epsilon, is_warm_start, exchange_num, always_select, splicing_type, sub_search);
        temp->covariance_update = covariance_update;
        algorithm_list_mul_dense[i] = temp;
      }
      else if (model_type == 6)
      {
        abessMultinomial<Eigen::MatrixXd> *temp = new abessMultinomial<Eigen::MatrixXd>(algorithm_type, model_type, max_iter, primary_model_fit_max_iter, primary_model_fit_epsilon, is_warm_start, exchange_num, always_select, splicing_type, sub_search);
        temp->approximate_Newton = approximate_Newton;
        algorithm_list_mul_dense[i] = temp;
      }
      else if(model_type == 8)
      {
        abessGamma<Eigen::MatrixXd> *temp = new abessGamma<Eigen::MatrixXd>(algorithm_type, model_type, max_iter, primary_model_fit_max_iter, primary_model_fit_epsilon, is_warm_start, exchange_num, always_select, splicing_type, sub_search);
        temp->approximate_Newton = approximate_Newton;
        algorithm_list_uni_dense[i] = temp;
      }
      else if (model_type == 10)
      {
        algorithm_list_uni_dense[i] = new abessRPCA<Eigen::MatrixXd>(algorithm_type, model_type, max_iter, primary_model_fit_max_iter, primary_model_fit_epsilon, is_warm_start, exchange_num, always_select, splicing_type, sub_search);
      }
    }
    else
    {
      if (model_type == 1)
      {
        abessLm<Eigen::SparseMatrix<double>> *temp = new abessLm<Eigen::SparseMatrix<double>>(algorithm_type, model_type, max_iter, primary_model_fit_max_iter, primary_model_fit_epsilon, is_warm_start, exchange_num, always_select, splicing_type, sub_search);
        temp->covariance_update = covariance_update;
        algorithm_list_uni_sparse[i] = temp;
      }
      else if (model_type == 2)
      {
        abessLogistic<Eigen::SparseMatrix<double>> *temp = new abessLogistic<Eigen::SparseMatrix<double>>(algorithm_type, model_type, max_iter, primary_model_fit_max_iter, primary_model_fit_epsilon, is_warm_start, exchange_num, always_select, splicing_type, sub_search);
        temp->approximate_Newton = approximate_Newton;
        algorithm_list_uni_sparse[i] = temp;
      }
      else if (model_type == 3)
      {
        abessPoisson<Eigen::SparseMatrix<double>> *temp = new abessPoisson<Eigen::SparseMatrix<double>>(algorithm_type, model_type, max_iter, primary_model_fit_max_iter, primary_model_fit_epsilon, is_warm_start, exchange_num, always_select, splicing_type, sub_search);
        temp->approximate_Newton = approximate_Newton;
        algorithm_list_uni_sparse[i] = temp;
      }
      else if (model_type == 4)
      {
        abessCox<Eigen::SparseMatrix<double>> *temp = new abessCox<Eigen::SparseMatrix<double>>(algorithm_type, model_type, max_iter, primary_model_fit_max_iter, primary_model_fit_epsilon, is_warm_start, exchange_num, always_select, splicing_type, sub_search);
        temp->approximate_Newton = approximate_Newton;
        algorithm_list_uni_sparse[i] = temp;
      }
      else if (model_type == 5)
      {
        abessMLm<Eigen::SparseMatrix<double>> *temp = new abessMLm<Eigen::SparseMatrix<double>>(algorithm_type, model_type, max_iter, primary_model_fit_max_iter, primary_model_fit_epsilon, is_warm_start, exchange_num, always_select, splicing_type, sub_search);
        temp->covariance_update = covariance_update;
        algorithm_list_mul_sparse[i] = temp;
      }
      else if (model_type == 6)
      {
        abessMultinomial<Eigen::SparseMatrix<double>> *temp = new abessMultinomial<Eigen::SparseMatrix<double>>(algorithm_type, model_type, max_iter, primary_model_fit_max_iter, primary_model_fit_epsilon, is_warm_start, exchange_num, always_select, splicing_type, sub_search);
        temp->approximate_Newton = approximate_Newton;
        algorithm_list_mul_sparse[i] = temp;
      }
      else if (model_type == 8)
      {
        abessGamma<Eigen::SparseMatrix<double>> *temp = new abessGamma<Eigen::SparseMatrix<double>>(algorithm_type, model_type, max_iter, primary_model_fit_max_iter, primary_model_fit_epsilon, is_warm_start, exchange_num, always_select, splicing_type, sub_search);
        temp->approximate_Newton = approximate_Newton;
        algorithm_list_uni_sparse[i] = temp;
      }
      else if (model_type == 10)
      {
        algorithm_list_uni_sparse[i] = new abessRPCA<Eigen::SparseMatrix<double>>(algorithm_type, model_type, max_iter, primary_model_fit_max_iter, primary_model_fit_epsilon, is_warm_start, exchange_num, always_select, splicing_type, sub_search);
      }
    }
  }

  List out_result;
  if (!sparse_matrix)
  {
    if (y.cols() == 1)
    {

      Eigen::VectorXd y_vec = y.col(0).eval();

      out_result = abessCpp<Eigen::VectorXd, Eigen::VectorXd, double, Eigen::MatrixXd>(x, y_vec, n, p, normalize_type,
                                                                                       weight, 
                                                                                       algorithm_type, model_type, max_iter, exchange_num,
                                                                                       path_type, is_warm_start,
                                                                                       ic_type, ic_coef, Kfold,
                                                                                       sequence,
                                                                                       lambda_seq,
                                                                                       s_min, s_max, 
                                                                                       lambda_min, lambda_max, nlambda,
                                                                                       screening_size, 
                                                                                       g_index,
                                                                                       primary_model_fit_max_iter, primary_model_fit_epsilon,
                                                                                       early_stop, 
                                                                                       thread,
                                                                                       sparse_matrix,
                                                                                       cv_fold_id,
                                                                                       algorithm_list_uni_dense);
    }
    else
    {

      out_result = abessCpp<Eigen::MatrixXd, Eigen::MatrixXd, Eigen::VectorXd, Eigen::MatrixXd>(x, y, n, p, normalize_type,
                                                                                                weight, 
                                                                                                algorithm_type, model_type, max_iter, exchange_num,
                                                                                                path_type, is_warm_start,
                                                                                                ic_type, ic_coef, Kfold,
                                                                                                sequence,
                                                                                                lambda_seq,
                                                                                                s_min, s_max, 
                                                                                                lambda_min, lambda_max, nlambda,
                                                                                                screening_size, 
                                                                                                g_index,
                                                                                                primary_model_fit_max_iter, primary_model_fit_epsilon,
                                                                                                early_stop, 
                                                                                                thread,
                                                                                                sparse_matrix,
                                                                                                cv_fold_id,
                                                                                                algorithm_list_mul_dense);
    }
  }
  else
  {

    Eigen::SparseMatrix<double> sparse_x(n, p);

    // std::vector<triplet> tripletList;
    // tripletList.reserve(x.rows());
    // for (int i = 0; i < x.rows(); i++)
    // {
    //   tripletList.push_back(triplet(int(x(i, 1)), int(x(i, 2)), x(i, 0)));
    // }
    // sparse_x.setFromTriplets(tripletList.begin(), tripletList.end());

    sparse_x.reserve(x.rows());
    for (int i = 0; i < x.rows(); i++)
    {
      sparse_x.insert(int(x(i, 1)), int(x(i, 2))) = x(i, 0);
    }
    sparse_x.makeCompressed();

    if (y.cols() == 1)
    {

      Eigen::VectorXd y_vec = y.col(0).eval();

      out_result = abessCpp<Eigen::VectorXd, Eigen::VectorXd, double, Eigen::SparseMatrix<double>>(sparse_x, y_vec, n, p, normalize_type,
                                                                                                   weight, 
                                                                                                   algorithm_type, model_type, max_iter, exchange_num,
                                                                                                   path_type, is_warm_start,
                                                                                                   ic_type, ic_coef, Kfold,
                                                                                                   sequence,
                                                                                                   lambda_seq,
                                                                                                   s_min, s_max, 
                                                                                                   lambda_min, lambda_max, nlambda,
                                                                                                   screening_size, 
                                                                                                   g_index,
                                                                                                   primary_model_fit_max_iter, primary_model_fit_epsilon,
                                                                                                   early_stop, 
                                                                                                   thread,
                                                                                                   sparse_matrix,
                                                                                                   cv_fold_id,
                                                                                                   algorithm_list_uni_sparse);
    }
    else
    {

      out_result = abessCpp<Eigen::MatrixXd, Eigen::MatrixXd, Eigen::VectorXd, Eigen::SparseMatrix<double>>(sparse_x, y, n, p, normalize_type,
                                                                                                            weight, 
                                                                                                            algorithm_type, model_type, max_iter, exchange_num,
                                                                                                            path_type, is_warm_start,
                                                                                                            ic_type, ic_coef, Kfold,
                                                                                                            sequence,
                                                                                                            lambda_seq,
                                                                                                            s_min, s_max, 
                                                                                                            lambda_min, lambda_max, nlambda,
                                                                                                            screening_size, 
                                                                                                            g_index,
                                                                                                            primary_model_fit_max_iter, primary_model_fit_epsilon,
                                                                                                            early_stop, 
                                                                                                            thread,
                                                                                                            sparse_matrix,
                                                                                                            cv_fold_id,
                                                                                                            algorithm_list_mul_sparse);
    }
  }

  for (unsigned int i = 0; i < algorithm_list_size; i++)
  {
    delete algorithm_list_uni_dense[i];
    delete algorithm_list_mul_dense[i];
    delete algorithm_list_uni_sparse[i];
    delete algorithm_list_mul_sparse[i];
  }

  return out_result;
};

// [[Rcpp::export]]
List abessPCA_API(Eigen::MatrixXd x,
                  int n,
                  int p,
                  int normalize_type,
                  Eigen::VectorXd weight,
                  Eigen::MatrixXd sigma,
                  int max_iter,
                  int exchange_num,
                  int path_type,
                  bool is_warm_start,
                  bool is_tune,
                  int ic_type,
                  double ic_coef,
                  int Kfold,
                  Eigen::VectorXi sequence,
                  int s_min,
                  int s_max,
                  double lambda_min,
                  double lambda_max,
                  int nlambda,
                  Eigen::VectorXi g_index,
                  Eigen::VectorXi always_select,
                  bool early_stop,
                  int thread,
                  bool sparse_matrix,
                  int splicing_type,
                  Eigen::VectorXi cv_fold_id,
                  int pca_num)
{
  /* this function for abessPCA only (model_type == 7) */

#ifdef _OPENMP
  // Eigen::initParallel();
  int max_thread = omp_get_max_threads();
  if (thread == 0 || thread > max_thread)
  {
    thread = max_thread;
  }

  Eigen::setNbThreads(thread);
  omp_set_num_threads(thread);
#endif

  int model_type = 7, algorithm_type = 6;
  Eigen::VectorXd lambda_seq = Eigen::VectorXd::Zero(1);
  int screening_size = -1;
  int sub_search = 0;
  int primary_model_fit_max_iter = 1;
  double primary_model_fit_epsilon = 1e-3;
  int pca_n = -1;
  if (!sparse_matrix && n != x.rows())
  {
    pca_n = n;
    n = x.rows();
  }
  Eigen::VectorXd y_vec = Eigen::VectorXd::Zero(n);

  //////////////////// function generate_algorithm_pointer() ////////////////////////////
  int algorithm_list_size = max(thread, Kfold);
  vector<Algorithm<Eigen::VectorXd, Eigen::VectorXd, double, Eigen::MatrixXd> *> algorithm_list_uni_dense(algorithm_list_size);
  vector<Algorithm<Eigen::VectorXd, Eigen::VectorXd, double, Eigen::SparseMatrix<double>> *> algorithm_list_uni_sparse(algorithm_list_size);
  for (int i = 0; i < algorithm_list_size; i++)
  {
    if (!sparse_matrix)
    {
      abessPCA<Eigen::MatrixXd> *temp = new abessPCA<Eigen::MatrixXd>(algorithm_type, model_type, max_iter, primary_model_fit_max_iter, primary_model_fit_epsilon, is_warm_start, exchange_num, always_select, splicing_type, sub_search);
      temp->is_cv = Kfold > 1;
      temp->pca_n = pca_n;
      temp->sigma = sigma;
      algorithm_list_uni_dense[i] = temp;
    }
    else
    {
      abessPCA<Eigen::SparseMatrix<double>> *temp = new abessPCA<Eigen::SparseMatrix<double>>(algorithm_type, model_type, max_iter, primary_model_fit_max_iter, primary_model_fit_epsilon, is_warm_start, exchange_num, always_select, splicing_type, sub_search);
      temp->is_cv = Kfold > 1;
      temp->pca_n = pca_n;
      temp->sigma = sigma;
      algorithm_list_uni_sparse[i] = temp;
    }
  }

  // call `abessCpp` for result
  List out_result;
  List out_result_next;
  int num = 0; 

  if (!sparse_matrix)
  {

    while (num++ < pca_num)
    {
      Eigen::VectorXi pca_support_size;
      if (is_tune)
      {
        pca_support_size = sequence;
      }
      else
      {
        pca_support_size = sequence.segment(num - 1, 1);
      }
      out_result_next = abessCpp<Eigen::VectorXd, Eigen::VectorXd, double, Eigen::MatrixXd>(x, y_vec, n, p, normalize_type,
                                                                                           weight, 
                                                                                           algorithm_type, model_type, max_iter, exchange_num,
                                                                                           path_type, is_warm_start,
                                                                                           ic_type, ic_coef, Kfold,
                                                                                           pca_support_size,
                                                                                           lambda_seq,
                                                                                           s_min, s_max, 
                                                                                           lambda_min, lambda_max, nlambda,
                                                                                           screening_size, 
                                                                                           g_index,
                                                                                           primary_model_fit_max_iter, primary_model_fit_epsilon,
                                                                                           early_stop, 
                                                                                           thread,
                                                                                           sparse_matrix,
                                                                                           cv_fold_id,
                                                                                           algorithm_list_uni_dense);
      Eigen::VectorXd beta_next;
#ifdef R_BUILD
      beta_next = out_result_next["beta"];
#else
      out_result_next.get_value_by_name("beta", beta_next);
#endif
      if (num == 1)
      {
        out_result = out_result_next;
      }
      else
      {
#ifdef R_BUILD
        Eigen::MatrixXd beta_new(p, num);
        Eigen::VectorXd temp = out_result["beta"];
        Eigen::Map<Eigen::MatrixXd> temp2(temp.data(), p, num - 1);
        beta_new << temp2, beta_next;
        out_result["beta"] = beta_new;
        // std::cout << "combine beta done!" << std::endl;
#else
        out_result.combine_beta(beta_next);
#endif
      }

      if (num < pca_num)
      {
        Eigen::MatrixXd temp = beta_next * beta_next.transpose();
        if (is_tune)
        {
          x -= x * temp;
        }
        else
        {
          Eigen::MatrixXd temp1 = temp * sigma;
          sigma += temp1 * temp - temp1 - temp1.transpose();
        }
      }
    }
  }
  else
  {

    Eigen::SparseMatrix<double> sparse_x(n, p);

    // std::vector<triplet> tripletList;
    // tripletList.reserve(x.rows());
    // for (int i = 0; i < x.rows(); i++)
    // {
    //   tripletList.push_back(triplet(int(x(i, 1)), int(x(i, 2)), x(i, 0)));
    // }
    // sparse_x.setFromTriplets(tripletList.begin(), tripletList.end());

    sparse_x.reserve(x.rows());
    for (int i = 0; i < x.rows(); i++)
    {
      sparse_x.insert(int(x(i, 1)), int(x(i, 2))) = x(i, 0);
    }
    sparse_x.makeCompressed();

    while (num++ < pca_num)
    {
      Eigen::VectorXi pca_support_size;
      if (is_tune)
      {
        pca_support_size = sequence;
      }
      else
      {
        pca_support_size = sequence.segment(num - 1, 1);
      }
      out_result_next = abessCpp<Eigen::VectorXd, Eigen::VectorXd, double, Eigen::SparseMatrix<double>>(sparse_x, y_vec, n, p, normalize_type,
                                                                                                       weight, 
                                                                                                       algorithm_type, model_type, max_iter, exchange_num,
                                                                                                       path_type, is_warm_start,
                                                                                                       ic_type, ic_coef, Kfold,
                                                                                                       pca_support_size,
                                                                                                       lambda_seq,
                                                                                                       s_min, s_max,
                                                                                                       lambda_min, lambda_max, nlambda,
                                                                                                       screening_size, 
                                                                                                       g_index,
                                                                                                       primary_model_fit_max_iter, primary_model_fit_epsilon,
                                                                                                       early_stop, 
                                                                                                       thread,
                                                                                                       sparse_matrix,
                                                                                                       cv_fold_id,
                                                                                                       algorithm_list_uni_sparse);
      Eigen::VectorXd beta_next;
#ifdef R_BUILD
      beta_next = out_result_next["beta"];
#else
      out_result_next.get_value_by_name("beta", beta_next);
#endif
      if (num == 1)
      {
        out_result = out_result_next;
      }
      else
      {
#ifdef R_BUILD
        Eigen::MatrixXd beta_new(p, num);
        Eigen::VectorXd temp = out_result["beta"];
        Eigen::Map<Eigen::MatrixXd> temp2(temp.data(), p, num - 1);
        beta_new << temp2, beta_next;
        out_result["beta"] = beta_new;
#else
        out_result.combine_beta(beta_next);
#endif
      }

      // update for next PCA
      if (num < pca_num)
      {
        Eigen::MatrixXd temp = beta_next * beta_next.transpose();
        if (is_tune)
        {
          sparse_x = sparse_x - sparse_x * temp;
        }
        else
        {
          Eigen::MatrixXd temp1 = temp * sigma;
          sigma += temp1 * temp - temp1 - temp1.transpose();
        }
      }
    }
  }

  for (unsigned int i = 0; i < algorithm_list_size; i++)
  {
    delete algorithm_list_uni_dense[i];
    delete algorithm_list_uni_sparse[i];
  }
  return out_result;
};

//  T1 for y, XTy, XTone
//  T2 for beta
//  T3 for coef0
//  T4 for X
//  <Eigen::VectorXd, Eigen::VectorXd, double, Eigen::MatrixXd> for Univariate Dense
//  <Eigen::VectorXd, Eigen::VectorXd, double, Eigen::SparseMatrix<double> > for Univariate Sparse
//  <Eigen::MatrixXd, Eigen::MatrixXd, Eigen::VectorXd, Eigen::MatrixXd> for Multivariable Dense
//  <Eigen::MatrixXd, Eigen::MatrixXd, Eigen::VectorXd, Eigen::SparseMatrix<double> > for Multivariable Sparse
template <class T1, class T2, class T3, class T4>
List abessCpp(T4 &x, T1 &y, int n, int p, int normalize_type,
              Eigen::VectorXd weight, 
              int algorithm_type, int model_type, int max_iter, int exchange_num,
              int path_type, bool is_warm_start,
              int ic_type, double ic_coef, int Kfold,
              Eigen::VectorXi sequence,
              Eigen::VectorXd lambda_seq,
              int s_min, int s_max, 
              double lambda_min, double lambda_max, int nlambda,
              int screening_size, 
              Eigen::VectorXi g_index,
              int primary_model_fit_max_iter, double primary_model_fit_epsilon,
              bool early_stop, 
              int thread,
              bool sparse_matrix,
              Eigen::VectorXi &cv_fold_id,
              vector<Algorithm<T1, T2, T3, T4> *> algorithm_list)
{
#ifndef R_BUILD
  std::srand(123);
#endif

  // int normalize_type;
  // switch (model_type)
  // {
  //     case 1: // gauss
  //     case 5: // mul-gauss
  //     case 7: // pca
  //         normalize_type = 1; break;
  //     case 2: // logi
  //     case 3: // poiss
  //     case 6: // mul-nomial
  //         normalize_type = 2; break;
  //     case 4: // cox
  //         normalize_type = 3; break;
  // };
  
  int algorithm_list_size = algorithm_list.size();
  int beta_size = algorithm_list[0]->get_beta_size(n, p); // number of candidate param

  // data packing
  Data<T1, T2, T3, T4> data(x, y, normalize_type, weight, g_index, sparse_matrix, beta_size);

  // screening
  Eigen::VectorXi screening_A; 
  if (screening_size >= 0) 
  {
    screening_A = screening<T1, T2, T3, T4>(data, algorithm_list, screening_size, beta_size, lambda_seq[0]);
  }

  // For CV:
  // 1:mask
  // 2:warm start save
  // 3:group_XTX
  Metric<T1, T2, T3, T4> *metric = new Metric<T1, T2, T3, T4>(ic_type, ic_coef, Kfold);
  if (Kfold > 1){
    metric->set_cv_train_test_mask(data, data.n, cv_fold_id);
    metric->set_cv_init_fit_arg(beta_size, data.M);
    // metric->set_cv_initial_model_param(Kfold, data.p);
    // metric->set_cv_initial_A(Kfold, data.p);
    // metric->set_cv_initial_coef0(Kfold, data.p);
    // if (model_type == 1)
    //   metric->cal_cv_group_XTX(data);
  }

  // calculate loss for each parameter parameter combination
  vector<Result<T2, T3>> result_list(Kfold);
  if (path_type == 1)
  {
#pragma omp parallel for
    for (int i = 0; i < Kfold; i++)
    {
      sequential_path_cv<T1, T2, T3, T4>(data, algorithm_list[i], metric, sequence, lambda_seq, early_stop, i, result_list[i]);
    }
  }
  else
  {
    // if (algorithm_type == 5 || algorithm_type == 3)
    // {
    //     double log_lambda_min = log(max(lambda_min, 1e-5));
    //     double log_lambda_max = log(max(lambda_max, 1e-5));

    //     result = pgs_path(data, algorithm, metric, s_min, s_max, log_lambda_min, log_lambda_max, powell_path, nlambda);
    // }
    gs_path<T1, T2, T3, T4>(data, algorithm_list, metric, s_min, s_max, sequence, lambda_seq, result_list[0]);
  }

  for (int k = 0; k < Kfold; k++)
  {
    algorithm_list[k]->clear_setting();
  }

  // Get bestmodel index && fit bestmodel
  int min_loss_index_row = 0, min_loss_index_col = 0, s_size = sequence.size(), lambda_size = lambda_seq.size();
  Eigen::Matrix<T2, Dynamic, Dynamic> beta_matrix(s_size, lambda_size);
  Eigen::Matrix<T3, Dynamic, Dynamic> coef0_matrix(s_size, lambda_size);
  Eigen::Matrix<VectorXd, Dynamic, Dynamic> bd_matrix(s_size, lambda_size);
  Eigen::MatrixXd ic_matrix(s_size, lambda_size);
  Eigen::MatrixXd test_loss_sum = Eigen::MatrixXd::Zero(s_size, lambda_size);
  Eigen::MatrixXd train_loss_matrix(s_size, lambda_size);
  Eigen::MatrixXd effective_number_matrix(s_size, lambda_size);

  if (Kfold == 1) 
  {
    beta_matrix = result_list[0].beta_matrix;
    coef0_matrix = result_list[0].coef0_matrix;
    ic_matrix = result_list[0].ic_matrix;
    train_loss_matrix = result_list[0].train_loss_matrix;
    effective_number_matrix = result_list[0].effective_number_matrix;
    ic_matrix.minCoeff(&min_loss_index_row, &min_loss_index_col);
  } 
  else 
  { 
    Eigen::MatrixXd test_loss_tmp;
    if (path_type == 1) //TODO
      for (int i = 0; i < Kfold; i++)
      {
        test_loss_tmp = result_list[i].test_loss_matrix;
        test_loss_sum = test_loss_sum + test_loss_tmp / Kfold;
      }
    else
      test_loss_sum = result_list[0].test_loss_matrix;
    test_loss_sum.minCoeff(&min_loss_index_row, &min_loss_index_col);

    Eigen::VectorXi used_algorithm_index = Eigen::VectorXi::Zero(algorithm_list_size);

    // refit on full data
#pragma omp parallel for
    for (int i = 0; i < sequence.size() * lambda_seq.size(); i++)
    {
      int s_index = i / lambda_seq.size();
      int lambda_index = i % lambda_seq.size();
      int algorithm_index = omp_get_thread_num();
      used_algorithm_index(algorithm_index) = 1;

      T2 beta_init;
      T3 coef0_init;
      coef_set_zero(beta_size, data.M, beta_init, coef0_init);
      Eigen::VectorXd bd_init = Eigen::VectorXd::Zero(data.g_num);

      // warmstart from CV's result
      if (path_type == 1){ // TODO
        for (int j = 0; j < Kfold; j++)
        {
          beta_init = beta_init + result_list[j].beta_matrix(s_index, lambda_index) / Kfold;
          coef0_init = coef0_init + result_list[j].coef0_matrix(s_index, lambda_index) / Kfold;
          bd_init = bd_init + result_list[j].bd_matrix(s_index, lambda_index) / Kfold;
        }
      }

      algorithm_list[algorithm_index]->update_sparsity_level(sequence(s_index));
      algorithm_list[algorithm_index]->update_lambda_level(lambda_seq(lambda_index));
      algorithm_list[algorithm_index]->update_beta_init(beta_init);
      algorithm_list[algorithm_index]->update_coef0_init(coef0_init);
      algorithm_list[algorithm_index]->update_bd_init(bd_init);
      algorithm_list[algorithm_index]->fit(data.x, data.y, data.weight, data.g_index, data.g_size, data.n, data.p, data.g_num);

      beta_matrix(s_index, lambda_index) = algorithm_list[algorithm_index]->get_beta();
      coef0_matrix(s_index, lambda_index) = algorithm_list[algorithm_index]->get_coef0();
      train_loss_matrix(s_index, lambda_index) = algorithm_list[algorithm_index]->get_train_loss();
      ic_matrix(s_index, lambda_index) = metric->ic(data.n, data.M, data.g_num, algorithm_list[algorithm_index]);
      effective_number_matrix(s_index, lambda_index) = algorithm_list[algorithm_index]->get_effective_number();
    }

    for (int i = 0; i < algorithm_list_size; i++)
      if (used_algorithm_index(i) == 1)
      {
        algorithm_list[i]->clear_setting();
      }
  }

  // best_fit_result (output)
  // int best_s = sequence(min_loss_index_row);
  double best_lambda = lambda_seq(min_loss_index_col);

  T2 best_beta;
  T3 best_coef0;
  double best_train_loss, best_ic, best_test_loss;

  best_beta = beta_matrix(min_loss_index_row, min_loss_index_col);
  best_coef0 = coef0_matrix(min_loss_index_row, min_loss_index_col);
  best_train_loss = train_loss_matrix(min_loss_index_row, min_loss_index_col);
  best_ic = ic_matrix(min_loss_index_row, min_loss_index_col);
  best_test_loss = test_loss_sum(min_loss_index_row, min_loss_index_col);

  //////////////Restore best_fit_result for normal//////////////
  restore_for_normal<T2, T3>(best_beta, best_coef0, beta_matrix, coef0_matrix, sparse_matrix,
                             data.normalize_type, data.n, data.x_mean, data.y_mean, data.x_norm);

  // List result;
  List out_result;
#ifdef R_BUILD
  out_result = List::create(Named("beta") = best_beta,
                            Named("coef0") = best_coef0,
                            Named("train_loss") = best_train_loss,
                            Named("ic") = best_ic,
                            Named("lambda") = best_lambda,
                            Named("beta_all") = beta_matrix,
                            Named("coef0_all") = coef0_matrix,
                            Named("train_loss_all") = train_loss_matrix,
                            Named("ic_all") = ic_matrix,
                            Named("effective_number_all") = effective_number_matrix,
                            Named("test_loss_all") = test_loss_sum);
  if (path_type == 2)
  {
    out_result.push_back(sequence, "sequence");
  }
#else
  out_result.add("beta", best_beta);
  out_result.add("coef0", best_coef0);
  out_result.add("train_loss", best_train_loss);
  out_result.add("test_loss", best_test_loss);
  out_result.add("ic", best_ic);
  out_result.add("lambda", best_lambda);
  // out_result.add("beta_all", beta_matrix);
  // out_result.add("coef0_all", coef0_matrix);
  // out_result.add("train_loss_all", train_loss_matrix);
  // out_result.add("ic_all", ic_matrix);
  // out_result.add("test_loss_all", test_loss_sum);
#endif

  // Restore best_fit_result for screening
  if (screening_size >= 0)
  {

    T2 beta_screening_A;
    T2 beta;
    T3 coef0;
    beta_size = algorithm_list[0]->get_beta_size(n, p);
    coef_set_zero(beta_size, data.M, beta, coef0);

#ifndef R_BUILD
    out_result.get_value_by_name("beta", beta_screening_A);
    slice_restore(beta_screening_A, screening_A, beta);
    out_result.add("beta", beta);
    out_result.add("screening_A", screening_A);
#else
    beta_screening_A = out_result["beta"];
    slice_restore(beta_screening_A, screening_A, beta);
    out_result["beta"] = beta;
    out_result.push_back(screening_A, "screening_A");
#endif
  }

  delete metric;
  return out_result;
}

#ifndef R_BUILD

void pywrap_abess(double *x, int x_row, int x_col, double *y, int y_row, int y_col, int n, int p, int normalize_type, double *weight, int weight_len, double *sigma, int sigma_row, int sigma_col,
                  int algorithm_type, int model_type, int max_iter, int exchange_num,
                  int path_type, bool is_warm_start,
                  int ic_type, double ic_coef, int Kfold,
                  int *gindex, int gindex_len,
                  int *sequence, int sequence_len,
                  double *lambda_sequence, int lambda_sequence_len,
                  int *cv_fold_id, int cv_fold_id_len,
                  int s_min, int s_max, 
                  double lambda_min, double lambda_max, int n_lambda,
                  int screening_size, 
                  int *always_select, int always_select_len, 
                  int primary_model_fit_max_iter, double primary_model_fit_epsilon,
                  bool early_stop, bool approximate_Newton,
                  int thread,
                  bool covariance_update,
                  bool sparse_matrix,
                  int splicing_type,
                  int sub_search,
                  int pca_num,
                  double *beta_out, int beta_out_len, double *coef0_out, int coef0_out_len, double *train_loss_out,
                  int train_loss_out_len, double *ic_out, int ic_out_len, double *nullloss_out, double *aic_out,
                  int aic_out_len, double *bic_out, int bic_out_len, double *gic_out, int gic_out_len, int *A_out,
                  int A_out_len, int *l_out)
{
  Eigen::MatrixXd x_Mat;
  Eigen::MatrixXd y_Mat;
  Eigen::MatrixXd sigma_Mat;
  Eigen::VectorXd weight_Vec;
  Eigen::VectorXi gindex_Vec;
  Eigen::VectorXi sequence_Vec;
  Eigen::VectorXd lambda_sequence_Vec;
  Eigen::VectorXi always_select_Vec;
  Eigen::VectorXi cv_fold_id_Vec;

  x_Mat = Pointer2MatrixXd(x, x_row, x_col);
  y_Mat = Pointer2MatrixXd(y, y_row, y_col);
  sigma_Mat = Pointer2MatrixXd(sigma, sigma_row, sigma_col);
  weight_Vec = Pointer2VectorXd(weight, weight_len);
  gindex_Vec = Pointer2VectorXi(gindex, gindex_len);
  sequence_Vec = Pointer2VectorXi(sequence, sequence_len);
  lambda_sequence_Vec = Pointer2VectorXd(lambda_sequence, lambda_sequence_len);
  always_select_Vec = Pointer2VectorXi(always_select, always_select_len);
  cv_fold_id_Vec = Pointer2VectorXi(cv_fold_id, cv_fold_id_len);

  List mylist;
  if (model_type == 7){
    bool is_tune = true;//TODO
    mylist = abessPCA_API(x_Mat, n, p, normalize_type, weight_Vec, sigma_Mat,
                          max_iter, exchange_num,
                          path_type, is_warm_start,
                          is_tune, ic_type, ic_coef, Kfold,
                          sequence_Vec,
                          s_min, s_max, 
                          lambda_min, lambda_max, n_lambda,
                          gindex_Vec,
                          always_select_Vec, 
                          early_stop, 
                          thread,
                          sparse_matrix,
                          splicing_type,
                          cv_fold_id_Vec,
                          pca_num);
  }else{
    mylist = abessGLM_API(x_Mat, y_Mat, n, p, normalize_type, weight_Vec, 
                            algorithm_type, model_type, max_iter, exchange_num,
                            path_type, is_warm_start,
                            ic_type, ic_coef, Kfold,
                            sequence_Vec,
                            lambda_sequence_Vec,
                            s_min, s_max, 
                            lambda_min, lambda_max, n_lambda,
                            screening_size, 
                            gindex_Vec,
                            always_select_Vec, 
                            primary_model_fit_max_iter, primary_model_fit_epsilon,
                            early_stop, approximate_Newton,
                            thread,
                            covariance_update,
                            sparse_matrix,
                            splicing_type,
                            sub_search,
                            cv_fold_id_Vec);
  }

  if (y_col == 1 && pca_num == 1)
  {
    Eigen::VectorXd beta;
    double coef0 = 0;
    double train_loss = 0;
    double ic = 0;
    mylist.get_value_by_name("beta", beta);
    mylist.get_value_by_name("coef0", coef0);
    mylist.get_value_by_name("train_loss", train_loss);
    mylist.get_value_by_name("ic", ic);

    VectorXd2Pointer(beta, beta_out);
    *coef0_out = coef0;
    *train_loss_out = train_loss;
    *ic_out = ic;
  }
  else
  {
    Eigen::MatrixXd beta;
    Eigen::VectorXd coef0;
    double train_loss = 0;
    double ic = 0;
    mylist.get_value_by_name("beta", beta);
    mylist.get_value_by_name("coef0", coef0);
    mylist.get_value_by_name("train_loss", train_loss);
    mylist.get_value_by_name("ic", ic);

    MatrixXd2Pointer(beta, beta_out);
    VectorXd2Pointer(coef0, coef0_out);
    train_loss_out[0] = train_loss;
    ic_out[0] = ic;
  }
}
#endif
