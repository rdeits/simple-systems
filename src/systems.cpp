#include <iostream>
#include <Eigen/Core>
#include <vector>
#include <stdexcept>
#include <unordered_map>

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

// private: // normally this would be private, but I'm exposing it to let me print the data without writing ostream << bindings for now
  Eigen::Matrix<Scalar, 2, 1> data;
};

class System {
public:
  virtual void dynamics(const Frame<double> &x, Frame<double> &xdot) const = 0;
  virtual void dynamics(const Frame<int> &x, Frame<int> &xdot) const = 0;
};

#define DYNAMICS_DISPATCH_BOILERPLATE \
  void dynamics(const Frame<double> &x, Frame<double> &xdot) const { \
    dynamics<double>(x, xdot); \
  } \
  void dynamics(const Frame<int> &x, Frame<int> &xdot) const { \
    dynamics<int>(x, xdot); \
  } \


class ExampleStaticSystem : public System {
public:
  template <typename Scalar> 
  void dynamics(const Frame<Scalar> &x, Frame<Scalar> &xdot) const {
    xdot.setValue(xdot.getIndex("q"), x.getValue(x.getIndex("qdot")));
    xdot.setValue(xdot.getIndex("qdot"), 1);
  }

  DYNAMICS_DISPATCH_BOILERPLATE
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
  void dynamics(const Frame<Scalar> &x, Frame<Scalar> &xdot) const {
    auto intermediate = std::unique_ptr<Frame<Scalar> >(xdot.clone());
    sys1.dynamics(x, *intermediate);
    sys2.dynamics(*intermediate, xdot);
  }

  DYNAMICS_DISPATCH_BOILERPLATE
};

int main() {
  auto sys1 = ExampleStaticSystem();
  auto x = ExampleStaticFrame<double>();
  auto xdot = ExampleStaticFrame<double>();

  static_assert(x.getIndexStatic("qdot") == 1, "index not statically found");

  sys1.dynamics(x, xdot);

  std::cout << xdot.data.transpose() << std::endl;

  auto sys2 = ExampleStaticSystem();

  auto sys3 = Chain(sys1, sys2);

  sys3.dynamics(x, xdot);
  std::cout << xdot.data.transpose() << std::endl;


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

}


