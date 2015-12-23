#include <iostream>
#include <Eigen/Core>
#include <vector>
#include <stdexcept>
#include <unordered_map>
#include <cstdlib>
// #include <cstring>

template <typename Scalar>
class Frame {
public:
  virtual Scalar getValue(const int index) const = 0;
  virtual void setValue(const int index, const Scalar &value) = 0;
  virtual int getIndex(const char * str) const = 0;
  virtual ~Frame() { }
  virtual Frame* clone() const = 0;
};

template <typename Scalar>
class VectorFrame: public Frame<Scalar> {
public:
  virtual Scalar getValue(const int index) const {
    return data[index];
  }

  virtual void setValue(const int index, const Scalar &value) {
    data[index] = value;
  }

  virtual int getIndex(const char * name) const {
    return coordinate_to_index[name];
  }

  virtual VectorFrame* clone() const {
    return new VectorFrame(*this);
  }

private:
  Eigen::Matrix<Scalar, Eigen::Dynamic, 1> data;
  std::unordered_map<std::string, int> coordinate_to_index;
  std::vector<std::string> coordinate_names;
};

constexpr bool static_strequal(const char * a, const char * b) {
  return (*a == 0 && *b == 0) ? true :
         (*a == 0) ? false :
         (*b == 0) ? false : 
         (*a != *b) ? false :
         static_strequal(a + 1, b + 1);
}

template <typename Scalar>
class ExampleStaticFrame: public Frame<Scalar> {

public:
  ExampleStaticFrame():
    data(Eigen::Matrix<Scalar, 2, 1>::Zero()) {}

  Scalar getValue(const int index) const {
    return data[index];
  }

  void setValue(const int index, const Scalar &value) {
    data[index] = value;
  }

  constexpr int getIndex(const char *name) const {
    return static_strequal(name, "q") ? 0 : 
           static_strequal(name, "qdot") ? 1 :
           throw std::runtime_error("index not found");
  }

  static constexpr int getIndexStatic(const char *name) {
    return static_strequal(name, "q") ? 0 : 
           static_strequal(name, "qdot") ? 1 :
           -1;
  }

  virtual ExampleStaticFrame* clone() const {
    return new ExampleStaticFrame(*this);
  }

// private:
  Eigen::Matrix<Scalar, 2, 1> data;
};

class System {
public:
  virtual void dynamics(const Frame<double> &x, Frame<double> &xdot) const = 0;
  virtual void dynamics(const Frame<int> &x, Frame<int> &xdot) const = 0;
};

class ExampleStaticSystem : public System {
public:
  template <typename Scalar> 
  void dynamicsImplementation(const Frame<Scalar> &x, Frame<Scalar> &xdot) const {
    xdot.setValue(xdot.getIndex("q"), x.getValue(x.getIndex("qdot")));
    xdot.setValue(xdot.getIndex("qdot"), 1);
  }

  void dynamics(const Frame<double> &x, Frame<double> &xdot) const {
    dynamicsImplementation(x, xdot);
  }
  void dynamics(const Frame<int> &x, Frame<int> &xdot) const {
    dynamicsImplementation(x, xdot);
  }
};

class Chain : public System {
private:
  const System &sys1;
  const System &sys2;

public:
  Chain(const System & sys1_, const System & sys2_):
    sys1(sys1_),
    sys2(sys2_) {}

  template <typename Scalar>
  void dynamicsImplementation(const Frame<Scalar> &x, Frame<Scalar> &xdot) const {
    auto intermediate = std::unique_ptr<Frame<Scalar> >(xdot.clone());
    sys1.dynamics(x, *intermediate);
    sys2.dynamics(*intermediate, xdot);
  }

  void dynamics(const Frame<double> &x, Frame<double> &xdot) const {
    dynamicsImplementation(x, xdot);
  }
  void dynamics(const Frame<int> &x, Frame<int> &xdot) const {
    dynamicsImplementation(x, xdot);
  }
};

// constexpr int getindex(const char * (str)) {
//   return static_strequal(str, "baz") ? 0 : 
//          static_strequal(str, "foobar") ? 1 :
//          -1;
// }

// void foo(const System &sys) {
//   std::cout << sys.dynamics(1.0) << std::endl;
//   std::cout << sys.dynamics(1) << std::endl;
// }

// int getindex_runtime(const char * (str)) {
//   if (static_strequal(str, "baz")) {
//     return 0;
//   } else if (static_strequal(str, "foobar")) {
//     return 1;
//   } else {
//     return -1;
//   }
// }

// struct FixedIndices {
//   int foobar;
//   int baz;
// };

int main() {
  auto sys1 = ExampleStaticSystem();
  auto x = ExampleStaticFrame<double>();
  auto xdot = ExampleStaticFrame<double>();

  static_assert(x.getIndexStatic("qdot") == 1, "index not statically found");

  sys1.dynamics(x, xdot);

  std::cout << xdot.data.transpose() << std::endl;

  // auto sys1 = Example();
  // std::cout << sys1.dynamics(1.0) << std::endl;
  // std::cout << sys1.dynamics(1) << std::endl;
  // foo(sys1);

  auto sys2 = ExampleStaticSystem();

  auto sys3 = Chain(sys1, sys2);

  sys3.dynamics(x, xdot);
  std::cout << xdot.data.transpose() << std::endl;

  // std::cout << sys3.dynamics(1.0) << std::endl;
  // std::cout << sys3.dynamics(1) << std::endl;


  // std::cout << static_strequal("foo", "f") << std::endl;
  // std::cout << static_strequal("foo", "foo") << std::endl;
  // std::cout << static_strequal("foo", "baz") << std::endl;
  // std::cout << static_strequal("f", "foo") << std::endl;

  // static_assert(static_strequal("baz", "baz"), "should be equal");
  // static_assert(getindex("foobar") == 1, "index");

  // int x[] = {5, 6};
  // auto indices = std::unordered_map<const char *, int> {{"baz", 0}, {"foobar", 1}};

  // // getindex: 0.59 s
  // // unordered_map operator[]: 1.46s
  // // FixedIndices struct: 0.00 s

  // auto fixed_indices = FixedIndices();
  // fixed_indices.foobar = 1;
  // fixed_indices.baz = 0;

  // int y = 0;
  // for (int i=0; i < 100000000; i++) {
  //   if (rand() % 2) {
  //     // y += x[fixed_indices.foobar];
  //     y += x[getindex("foobar")];
  //     // y += x[getindex_runtime("foobar")];
  //     // y += x[indices["foobar"]];
  //   } else {
  //     // y += x[fixed_indices.baz];
  //     y += x[getindex("baz")];
  //     // y += x[getindex_runtime("baz")];
  //     // y += x[indices["baz"]];
  //   }
  // }
  // std::cout << y << std::endl;

  // constexpr auto baz = "baz";
  // constexpr auto foobar = "foobar";
  // std::cout << x[getindex(baz)] << std::endl;
  // std::cout << x[getindex(foobar)] << std::endl;
  // std::cout << getindex("foo") << std::endl;
  // std::cout << getindex("baz") << std::endl;
  // std::cout << getindex("foobar") << std::endl;

  // if (s == "baz") {
  //   std::cout << "1" << std::endl;
  // } else if (s == "foobar") {
  //   std::cout << "2" << std::endl;
  // }
  // std::cout << "foobar";
}

// class Frame {
// public:
//   // virtual int getCoordinateIndex(const std::string & name);
// };

// int getCoordinateIndex(const Frame & frame, const std::string & name) {
//   throw std::runtime_error("not implemented");
// }


// template <typename Scalar>
// class VectorFrame : public Frame {
// public:
//   Eigen::Matrix<Scalar, Eigen::Dynamic, 1> data;

//   std::vector<std::string> coordinate_names;

//   VectorFrame(Eigen::Matrix<Scalar, Eigen::Dynamic, 1> data_, std::vector<std::string> coordinate_names_):
//     data(data_),
//     coordinate_names(coordinate_names_) {
//     if (data_.size() != coordinate_names.size()) {
//       throw std::runtime_error("lengths don't match");
//     }
//   }
// };

// template <typename Scalar>
// int getCoordinateIndex(const VectorFrame<Scalar> &frame, const std::string & name) {
//   auto match = std::find(frame.coordinate_names.begin(), frame.coordinate_names.end(), name);
//   if (match != frame.coordinate_names.end()) {
//     return match - frame.coordinate_names.begin();
//   } else {
//     std::cerr << "coordinate name not found: " << name << std::endl;
//     throw std::runtime_error("coordinate name not found");
//   }
// }

//   Scalar get(int index) {
//     return data[index];
//   }

//   void set(int index, Scalar value) {
//     data[index] = value;
//   }
// };



// class System {

// public:
//   Frame dynamics(double t, Frame x, Frame u);

//   Frame output(double t, Frame x, Frame u);
// };

// class DoubleIntegrator : public System {
//   Frame dynamics(double t, Frame x, Frame u) {
//     auto xdot = VectorFrame<double>(x.data, x.coordinate_names);
//     xdot.set(xdot.getCoordinateIndex("z"), x.get(x.getCoordinateIndex("zdot")));
//     xdot.set(xdot.getCoordinateIndex("zdot"), u.get(u.getCoordinateIndex("z")));
//     return xdot;
//   }
// };

// int main(int argc, char ** argv) {
//   auto x0 = Eigen::VectorXd(2);
//   x0 << 0, 1;
//   std::vector<std::string> x_coords = {"z", "zdot"};
//   x0_frame = VectorFrame(x0, x_coords);

//   auto u = Eigen::VectorXd(1);
//   u << 2;
//   std::vector<std::string> u_coords = {"z"};
//   u_frame = VectorFrame(u, u_coords);

//   auto sys = DoubleIntegrator();
//   auto xdot = sys.dynamics(0, x0_frame, u_frame);
//   std::cout << "xdot: " << xdot.data.transpose();

// }


