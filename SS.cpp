#include "SS.h"
#include "functional"

void Spline::SS::master_element(size_t segment_num, const double &x, double &ksi) const
{ ksi = 2.0 * (x - points[segment_num].get_x()) / (points[segment_num + 1].get_x() - points[segment_num].get_x()) - 1.0; }

double Spline::SS::basic(char num, const double &ksi) const
{
   switch (num)
   {
   case 1:
      return 0.5 * (1 - ksi);
      break;
   case 2:
      return 0.5 * (1 + ksi);
      break;
   default:
      throw std::exception("Wrong function number");
      break;
   }
}

double Spline::SS::der_basic(char num, const double &ksi) const
{
   switch (num)
   {
   case 1:
      return -0.5;
      break;
   case 2:
      return 0.5;
      break;
   default:
      throw std::exception("Wrong derivative number");
      break;
   }
}

void Spline::SS::update(const std::vector<Point> &p, const std::vector<double>& value)
{
   points.clear();
   for (auto elem : p)
      points.push_back(elem);

   size_t segment_num = p.size() - 1;
   alpha.resize(segment_num + 1);
   std::vector<double> a, b, c;
   a.resize(segment_num + 1);
   b.resize(segment_num + 1);
   c.resize(segment_num + 1);

   std::function<void(size_t segment_num, const Point &p, const double &val, const double &W)>
   assembling = [&](size_t i, const Point &p, const double &value, const double &weight)
   {
      double elem = p.get_x(), ksi;
      master_element(i, elem, ksi);

      double first = basic(1, ksi), second = basic(2, ksi);
      b[i] += weight * first * first;
      b[i + 1] += weight * second * second;
      a[i + 1] += weight * first * second;
      c[i] += weight * second * first;
      alpha[i] += weight * first * value;
      alpha[i + 1] += weight * second * value;
   };

   for (size_t i = 0; i < segment_num; i++)
   {
      assembling(i, points[i], value[i], 1.0);
      double h = points[i + 1].get_x() - points[i].get_x();
      b[i] += 4.0 / h * smooth;
      b[i + 1] += 4.0 / h * smooth;
      a[i + 1] -= 4.0 / h * smooth;
      c[i] -= 4.0 / h * smooth;
   }

   assembling(segment_num - 1, points[segment_num], value[segment_num], 1.0);

   for (size_t j = 1; j < segment_num; j++)
   {
      b[j] -= a[j] / b[j - 1] * c[j - 1];
      alpha[j] -= a[j] / b[j - 1] * alpha[j - 1];
   }

   alpha[segment_num] /= b[segment_num];
   for (size_t j = segment_num - 1; j >= 0; j--)
      alpha[j] = (alpha[j] - alpha[j + 1] * c[j]) / b[j];
}

void Spline::SS::get_value(const Point &p, double *result) const
{
   double eps = 1e-7;
   size_t segment_num = points.size() - 1;
   double elem = p.get_x();

   for (size_t i = 0; i < segment_num; i++)
      if (elem > points[i].get_x() && elem < points[i + 1].get_x() || fabs(elem - points[i].get_x()) < eps ||
         fabs(elem - points[i + 1].get_x()) < eps)
      {
         double h = points[i + 1].get_x() - points[i].get_x(), ksi;
         master_element(i, elem, ksi);
         result[0] = alpha[i] * basic(1, ksi) + alpha[i + 1] * basic(2, ksi);
         result[1] = (alpha[i] * der_basic(1, ksi) + alpha[i + 1] * der_basic(2, ksi)) * 2.0 / h;
         result[2] = 0.0;
         return;
      }
   throw std::exception("The point wasn't found");
}