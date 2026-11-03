#include "storm/math/transform2d.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
namespace Storm::Math {
namespace {
[[nodiscard]] Error bad_input(const char* message){return Error::make(ErrorCode::InvalidArgument,message);}
[[nodiscard]] float checked_float(double value) noexcept {
 constexpr double max=static_cast<double>(std::numeric_limits<float>::max());
 if(std::isnan(value))return std::numeric_limits<float>::quiet_NaN();
 if(!std::isfinite(value)||value>max||value< -max)
  return std::signbit(value) ? -std::numeric_limits<float>::infinity() : std::numeric_limits<float>::infinity();
 return static_cast<float>(value);
}
}
bool Vector2::is_finite() const noexcept{return std::isfinite(x)&&std::isfinite(y);}
Matrix3 Matrix3::identity() noexcept{Matrix3 m;m.values_[0]=1;m.values_[4]=1;m.values_[8]=1;return m;}
float Matrix3::at(std::size_t r,std::size_t c) const{
 if(r>=3U||c>=3U)throw std::out_of_range("Matrix3 indices must be in [0, 3)");
 return values_[(c*3U)+r];
}
bool Matrix3::is_finite() const noexcept{
 return std::all_of(values_.begin(),values_.end(),[](float x){return std::isfinite(x);});
}
Result<Matrix3> Matrix3::multiply_checked(const Matrix3& lhs,const Matrix3& rhs){
 if(!lhs.is_finite()||!rhs.is_finite())return Result<Matrix3>::failure(bad_input("Matrix3 multiplication requires finite inputs"));
 constexpr double max=static_cast<double>(std::numeric_limits<float>::max());
 Matrix3 out;
 for(std::size_t c=0;c<3U;++c)for(std::size_t r=0;r<3U;++r){
  double sum=0.0;
  for(std::size_t k=0;k<3U;++k)sum+=static_cast<double>(lhs.values_[(k*3U)+r])*static_cast<double>(rhs.values_[(c*3U)+k]);
  if(!std::isfinite(sum)||sum>max||sum< -max)return Result<Matrix3>::failure(bad_input("Matrix3 multiplication exceeds the finite float range"));
  out.values_[(c*3U)+r]=static_cast<float>(sum);
 }
 return Result<Matrix3>::success(out);
}
Result<Vector2> Matrix3::transform_point(Vector2 point) const{
 if(!is_finite()||!point.is_finite())return Result<Vector2>::failure(bad_input("Matrix3 and point must contain finite values"));
 const double x=static_cast<double>(point.x),y=static_cast<double>(point.y);
 const double tx=static_cast<double>(values_[0])*x+static_cast<double>(values_[3])*y+static_cast<double>(values_[6]);
 const double ty=static_cast<double>(values_[1])*x+static_cast<double>(values_[4])*y+static_cast<double>(values_[7]);
 const double tw=static_cast<double>(values_[2])*x+static_cast<double>(values_[5])*y+static_cast<double>(values_[8]);
 constexpr double min_w=1.0e-12;
 constexpr double max=static_cast<double>(std::numeric_limits<float>::max());
 if(!std::isfinite(tw)||std::fabs(tw)<=min_w)return Result<Vector2>::failure(bad_input("Matrix3 point transform has a zero or invalid homogeneous coordinate"));
 const double ox=tx/tw,oy=ty/tw;
 if(!std::isfinite(ox)||!std::isfinite(oy)||ox>max||ox< -max||oy>max||oy< -max)return Result<Vector2>::failure(bad_input("Transformed point exceeds the finite Vector2 range"));
 return Result<Vector2>::success(Vector2{static_cast<float>(ox),static_cast<float>(oy)});
}
Matrix3 operator*(const Matrix3& a,const Matrix3& b) noexcept{
 Matrix3 out;
 for(std::size_t c=0;c<3U;++c)for(std::size_t r=0;r<3U;++r){
  double sum=0;
  for(std::size_t k=0;k<3U;++k)
   sum+=static_cast<double>(a.values_[(k*3U)+r])*static_cast<double>(b.values_[(c*3U)+k]);
  out.values_[(c*3U)+r]=checked_float(sum);
 }
 return out;
}
bool Transform2D::is_valid() const noexcept{
 return translation.is_finite()&&std::isfinite(rotation_radians)&&scale.is_finite();
}
Result<Matrix3> Transform2D::to_matrix() const{
 if(!is_valid())return Result<Matrix3>::failure(bad_input("Transform2D fields must be finite"));
 const double angle=static_cast<double>(rotation_radians),cs=std::cos(angle),sn=std::sin(angle);
 const double sx=static_cast<double>(scale.x),sy=static_cast<double>(scale.y);
 const std::array<double,9U> v{cs*sx,sn*sx,0,-sn*sy,cs*sy,0,
  static_cast<double>(translation.x),static_cast<double>(translation.y),1};
 constexpr double max=static_cast<double>(std::numeric_limits<float>::max());
 for(double x:v)if(!std::isfinite(x)||x>max||x< -max)
  return Result<Matrix3>::failure(bad_input("Transform2D exceeds Matrix3 numeric range"));
 Matrix3 m;for(std::size_t i=0;i<v.size();++i)m.values_[i]=static_cast<float>(v[i]);
 return Result<Matrix3>::success(m);
}
} // namespace Storm::Math
