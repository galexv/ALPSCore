/*****************************************************************************
 *
 * ALPS Project: Algorithms and Libraries for Physics Simulations
 *
 * ALPS Libraries
 *
 * Copyright (C) 2008-2010 by Matthias Troyer <troyer@itp.phys.ethz.ch>,
 *                            Lukas Gamper <gamperl -at- gmail.com>
 *
 * This software is part of the ALPS libraries, published under the ALPS
 * Library License; you can use, redistribute it and/or modify it under
 * the terms of the license, either version 1 or (at your option) any later
 * version.
 * 
 * You should have received a copy of the ALPS Library License along with
 * the ALPS Libraries; see the file LICENSE.txt. If not, the license is also
 * available from http://alps.comp-phys.org/.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT 
 * SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE 
 * FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE, 
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 *
 *****************************************************************************/

#ifndef ALPS_HDF5_HPP
#define ALPS_HDF5_HPP

#ifndef _HDF5USEDLL_
# define _HDF5USEDLL_
#endif
#ifndef _HDF5USEHLDLL_
# define _HDF5USEHLDLL_
#endif

#include <alps/config.h>

#ifdef ALPS_USE_NGS
	#include <alps/ngs/mchdf5.hpp>
#endif

#include <map>
#include <set>
#include <list>
#include <deque>
#include <vector>
#include <string>
#include <complex>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <cassert>
#include <valarray>
#include <typeinfo>
#include <iostream>
#include <stdexcept>

#include <boost/ref.hpp>
#include <boost/any.hpp>
#include <boost/array.hpp>
#include <boost/config.hpp>
#include <boost/mpl/or.hpp>
#include <boost/mpl/if.hpp>
#include <boost/utility.hpp>
#include <boost/mpl/and.hpp>
#include <boost/integer.hpp>
#include <boost/optional.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/filesystem.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/multi_array.hpp>
#include <boost/type_traits.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/shared_array.hpp>
#include <boost/static_assert.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/numeric/ublas/vector.hpp>
#include <boost/ptr_container/ptr_map.hpp>
#include <boost/numeric/bindings/ublas.hpp>
#include <boost/type_traits/add_reference.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <hdf5.h>

namespace alps {
    #ifndef ALPS_HDF5_SZIP_BLOCK_SIZE
        #define ALPS_HDF5_SZIP_BLOCK_SIZE 32
    #endif

    namespace hdf5 {
        #define ALPS_HDF5_STRINGIFY(arg) ALPS_HDF5_STRINGIFY_HELPER(arg)
        #define ALPS_HDF5_STRINGIFY_HELPER(arg) #arg
        #define ALPS_HDF5_THROW_ERROR(error, message)                                                                                                      \
            {                                                                                                                                              \
                std::ostringstream buffer;                                                                                                                 \
                buffer << "Error in " << __FILE__ << " on " << ALPS_HDF5_STRINGIFY(__LINE__) << " in " << __FUNCTION__ << ":" << std::endl << message;     \
                throw ( error (buffer.str()));                                                                                                             \
            }
        #define ALPS_HDF5_THROW_RUNTIME_ERROR(message)                                                                                                     \
            ALPS_HDF5_THROW_ERROR(std::runtime_error, message)
        #define ALPS_HDF5_THROW_RANGE_ERROR(message)                                                                                                       \
            ALPS_HDF5_THROW_ERROR(std::range_error, message)

        namespace detail {
            namespace internal_state_type {
                typedef enum { CREATE, PLACEHOLDER } type;
            }
            struct internal_log_type {
                char * time;
                char * name;
            };
            struct internal_complex_type {
               double r;
               double i;
            };

            #ifdef ALPS_HDF5_NO_LEXICAL_CAST
                template<typename U, typename T> inline U convert(T arg) {
                    return static_cast<U>(arg);
                }
                #define ALPS_HDF5_CONVERT_STRING(T, c)                                                                                                     \
                    template<> inline std::string convert<std::string, T >( T arg) {                                                                       \
                        char buffer[255];                                                                                                                  \
                        if (sprintf(buffer, "%" c, arg) < 0)                                                                                               \
                            ALPS_HDF5_THROW_RUNTIME_ERROR("error converting to string");                                                                   \
                        return buffer;                                                                                                                     \
                    }                                                                                                                                      \
                    template<> inline T convert< T, std::string>(std::string arg) {                                                                        \
                        T value;                                                                                                                           \
                        if (sscanf(arg.c_str(), "%" c, &value) < 0)                                                                                        \
                            ALPS_HDF5_THROW_RUNTIME_ERROR("error converting from to string");                                                              \
                        return value;                                                                                                                      \
                    }
                ALPS_HDF5_CONVERT_STRING(short, "hd")
                ALPS_HDF5_CONVERT_STRING(int, "d")
                ALPS_HDF5_CONVERT_STRING(long, "ld")
                ALPS_HDF5_CONVERT_STRING(unsigned short, "hu")
                ALPS_HDF5_CONVERT_STRING(unsigned int, "u")
                ALPS_HDF5_CONVERT_STRING(unsigned long, "lu")
                ALPS_HDF5_CONVERT_STRING(float, "f")
                ALPS_HDF5_CONVERT_STRING(double, "lf")
                ALPS_HDF5_CONVERT_STRING(long double, "Lf")
                #ifndef BOOST_NO_LONG_LONG
                    ALPS_HDF5_CONVERT_STRING(long long, "Ld")
                    ALPS_HDF5_CONVERT_STRING(unsigned long long, "Lu")
                #endif
                #undef ALPS_HDF5_CONVERT_STRING
                #define ALPS_HDF5_CONVERT_STRING_CHAR(T, U)                                                                                                \
                    template<> inline std::string convert<std::string, T >( T arg) {                                                                       \
                        return convert<std::string>(static_cast< U >(arg));                                                                                \
                    }                                                                                                                                      \
                    template<> inline T convert<T, std::string>(std::string arg) {                                                                         \
                        return static_cast< T >(convert< U >(arg));                                                                                        \
                    }
                ALPS_HDF5_CONVERT_STRING_CHAR(char, short)
                ALPS_HDF5_CONVERT_STRING_CHAR(signed char, short)
                ALPS_HDF5_CONVERT_STRING_CHAR(unsigned char, unsigned short)
                #undef ALPS_HDF5_CONVERT_STRING_CHAR
            #else
                template<typename U, typename T> inline U convert(T arg) {
                    return boost::lexical_cast<U>(arg);
                }
            #endif
        }

        template<typename T> std::vector<hsize_t> call_get_extent(T const &);
        template<typename T> void call_set_extent(T &, std::vector<std::size_t> const &);
        template<typename T> std::vector<hsize_t> call_get_offset(T const &);
        template<typename T> bool call_is_vectorizable(T const &);
        template<typename T, typename U> U const * call_get_data(
              std::vector<U> &
            , T const &
            , std::vector<hsize_t> const &
            , boost::optional<std::vector<hsize_t> > const & t = boost::none_t()
        );
        template<typename T, typename U> void call_set_data(T &, std::vector<U> const &, std::vector<hsize_t> const &, std::vector<hsize_t> const &);

        template<typename T> struct serializable_type { typedef T type; };
        template<typename T> struct native_type { typedef T type; };
        template<typename T> struct is_native : public boost::mpl::and_<
            typename boost::is_scalar<T>::type,
            typename boost::mpl::not_<typename boost::is_enum<T>::type>::type
        >::type {};
        template<typename T> struct is_cplx : public boost::mpl::false_ {};
        template<typename T> std::vector<hsize_t> get_extent(T const &) { return std::vector<hsize_t>(1, 1); }
        template<typename T> void set_extent(T &, std::vector<std::size_t> const &) {}
        template<typename T> std::vector<hsize_t> get_offset(T const &) { return std::vector<hsize_t>(1, 1); }
        template<typename T> bool is_vectorizable(T const &) { 
            return boost::is_same<T, std::string>::value || boost::is_same<T, detail::internal_state_type::type>::value || (boost::is_scalar<T>::value && !boost::is_enum<T>::value); }
        template<typename T> typename boost::disable_if<typename boost::mpl::or_<
            typename boost::is_same<T, bool>::type, 
            typename boost::is_same<T, std::string>::type, 
            typename boost::is_same<T, detail::internal_state_type::type>::type
        >::type, typename serializable_type<T>::type const *>::type get_data(
              std::vector<typename serializable_type<T>::type> & m
            , T const & v
            , std::vector<hsize_t> const &
            , std::vector<hsize_t> const & = std::vector<hsize_t>()
        ) {
            return &v;
        }
        namespace detail {
            template<typename T, typename U> void set_data_impl(T & v, std::vector<U> const & u, std::vector<hsize_t> const & s, std::vector<hsize_t> const & c, boost::mpl::false_) {
                if (s.size() != 1 || c.size() != 1 || c[0] == 0 || u.size() < c[0])
                    ALPS_HDF5_THROW_RANGE_ERROR("invalid data size")
                std::copy(u.begin(), u.begin() + static_cast<std::size_t>(c[0]), &v + static_cast<std::size_t>(s[0]));
            }
            template<typename T, typename U> void set_data_impl(T &, std::vector<U> const &, std::vector<hsize_t> const &, std::vector<hsize_t> const &, boost::mpl::true_) { 
                ALPS_HDF5_THROW_RUNTIME_ERROR("invalid type conversion")
            }
        }
        template<typename T, typename U> void set_data(T & v, std::vector<U> const & u, std::vector<hsize_t> const & s, std::vector<hsize_t> const & c) {
            detail::set_data_impl(v, u, s, c, typename boost::mpl::or_<
                  typename boost::is_same<U, std::complex<double> >::type
                , typename boost::is_same<U, char *>::type
                , typename boost::is_enum<T>::type
                , typename boost::mpl::not_<typename boost::is_scalar<T>::type>::type
            >::type());
        }

        template<> struct serializable_type<bool> { typedef boost::int8_t type; };
        template<> struct native_type<bool> { typedef boost::int8_t type; };
        template<> struct is_native<bool> : public boost::mpl::true_ {};
        template<> struct is_native<bool const> : public boost::mpl::true_ {};
        template<typename T> typename boost::enable_if<
            typename boost::is_same<T, bool>::type, typename serializable_type<T>::type const *
        >::type get_data(
              std::vector<serializable_type<bool>::type> & m
            , T const & v
            , std::vector<hsize_t> const &
            , std::vector<hsize_t> const & = std::vector<hsize_t>()
        ) {
            m.resize(1);
            return &(m[0] = v);
        }

        template<> struct serializable_type<std::string> { typedef char const * type; };
        template<> struct native_type<std::string> { typedef std::string type; };
        template<> struct is_native<std::string> : public boost::mpl::true_ {};
        template<> struct is_native<std::string const> : public boost::mpl::true_ {};
        template<typename T> typename boost::enable_if<
            typename boost::is_same<T, std::string>::type, typename serializable_type<T>::type const *
        >::type get_data(
              std::vector<serializable_type<std::string>::type> & m
            , T const & v
            , std::vector<hsize_t> const &
            , std::vector<hsize_t> const & t = std::vector<hsize_t>()
        ) {
            m.resize(1);
            return &(m[0] = v.c_str());
        }
        template<typename T> typename boost::disable_if<typename boost::mpl::or_<
            typename boost::is_same<T, std::complex<double> >::type,
            typename boost::is_same<T, char *>::type
        >::type>::type set_data(std::string & v, std::vector<T> const & u, std::vector<hsize_t> const & s, std::vector<hsize_t> const & c) { 
            if (s.size() != 1 || c.size() != 1 || c[0] == 0 || u.size() < c[0])
                ALPS_HDF5_THROW_RANGE_ERROR("invalid data size")
            for (std::string * w = &v + s[0]; w != &v + s[0] + c[0]; ++w)
                *w = detail::convert<std::string>(u[s[0] + (w - &v)]);
        }
        template<typename T> typename boost::enable_if<
            typename boost::is_same<T, char *>::type
        >::type set_data(std::string & v, std::vector<T> const & u, std::vector<hsize_t> const & s, std::vector<hsize_t> const & c) { 
            if (s.size() != 1 || c.size() != 1 || c[0] == 0 || u.size() < c[0])
                ALPS_HDF5_THROW_RANGE_ERROR("invalid data size")
            std::copy(u.begin(), u.begin() + c[0], &v);
        }
        template<typename T> typename boost::enable_if<
            typename boost::is_same<T, std::complex<double> >::type
        >::type set_data(std::string &, std::vector<T> const &, std::vector<hsize_t> const &, std::vector<hsize_t> const &) { 
            ALPS_HDF5_THROW_RUNTIME_ERROR("invalid type conversion")
        }

        template<> struct serializable_type<detail::internal_state_type::type> { typedef detail::internal_state_type::type type; };
        template<> struct native_type<detail::internal_state_type::type> { typedef detail::internal_state_type::type type; };
        template<> struct is_native<detail::internal_state_type::type> : public boost::mpl::true_ {};
        template<> struct is_native<detail::internal_state_type::type const> : public boost::mpl::true_ {};
        template<typename T> typename boost::enable_if<
            typename boost::is_same<T, detail::internal_state_type::type>::type, typename serializable_type<T>::type const *
        >::type get_data(
              std::vector<serializable_type<detail::internal_state_type::type>::type> & m
            , T const & v
            , std::vector<hsize_t> const &
            , std::vector<hsize_t> const & = std::vector<hsize_t>()
        ) {
            m.resize(1);
            return &(m[0] = v);
        }
        template<typename T> void set_data(detail::internal_state_type::type & v, std::vector<T> const & u, std::vector<hsize_t> const & s, std::vector<hsize_t> const &) {
            v = u.front();
        }

        template<typename T> bool is_vectorizable(std::complex<T> const &) { return true; }
        template<typename T> struct is_cplx<std::complex<T> > : public boost::mpl::true_ {};
        #ifdef ALPS_HDF5_WRITE_PYTHON_COMPATIBLE_COMPLEX
            template<typename T> struct serializable_type<std::complex<T> > { typedef detail::internal_complex_type type; };
            template<typename T> struct native_type<std::complex<T> > { typedef std::complex<T> type; };
            template<typename T> struct is_native<std::complex<T> > : public boost::mpl::true_ {};
            template<typename T> struct is_native<std::complex<T> const > : public boost::mpl::true_ {};
            template<typename T> std::vector<hsize_t> get_offset(std::complex<T> const &) { return std::vector<hsize_t>(1, 1); }
            template<typename T> void set_extent(std::complex<T>  &, std::vector<std::size_t> const & s) {
                if (!is_native<T>::value)
                    ALPS_HDF5_THROW_RUNTIME_ERROR("complex can only be build over scalar data types")
                if(s.size() > 0)
                    ALPS_HDF5_THROW_RANGE_ERROR("invalid data size")
            }
            template<typename T> std::vector<hsize_t> get_extent(std::complex<T> const &) { return std::vector<hsize_t>(1, 1); }
            template<typename T> typename serializable_type<std::complex<T> >::type const * get_data(
                  std::vector<typename serializable_type<std::complex<T> >::type> & m
                , std::complex<T> const & v
                , std::vector<hsize_t> const &
                , std::vector<hsize_t> const & t = std::vector<hsize_t>(1, 1)
            ) {
                if (t.size() != 1 || t[0] == 0)
                    ALPS_HDF5_THROW_RANGE_ERROR("invalid data size")
                m.resize(t[0]);
                for (std::complex<T> const * u = &v; u != &v + t[0]; ++u) {
                    detail::internal_complex_type c = { u->real(), u->imag() };
                    m[u - &v] = c;
                }
                return &m[0];
            }
        #else
            template<typename T> struct serializable_type<std::complex<T> > {
                typedef typename serializable_type<typename boost::remove_const<T>::type>::type type;
            };
            template<typename T> struct native_type<std::complex<T> > {
                typedef typename native_type<typename boost::remove_const<T>::type>::type type;
            };
            template<typename T> struct is_native<std::complex<T> > : public boost::mpl::false_ {};
            template<typename T> struct is_native<std::complex<T> const > : public boost::mpl::false_ {};
            template<typename T> std::vector<hsize_t> get_offset(std::complex<T> const &) { return std::vector<hsize_t>(1, 2); }
            template<typename T> void set_extent(std::complex<T>  &, std::vector<std::size_t> const & s) {
                if (!is_native<T>::value)
                    ALPS_HDF5_THROW_RUNTIME_ERROR("complex can only be build over scalar data types")
                if(s.size() != 1)
                    ALPS_HDF5_THROW_RANGE_ERROR("invalid data size")
            }
            template<typename T> std::vector<hsize_t> get_extent(std::complex<T> const & v) {
                if (!is_native<T>::value)
                    ALPS_HDF5_THROW_RUNTIME_ERROR("complex can only be build over scalar data types")
                return std::vector<hsize_t>(1, 2);
            }
            template<typename T> typename serializable_type<std::complex<T> >::type const * get_data(
                  std::vector<typename serializable_type<std::complex<T> >::type> & m
                , std::complex<T> const & v
                , std::vector<hsize_t> const & s
                , std::vector<hsize_t> const & = std::vector<hsize_t>()
            ) {
                return reinterpret_cast<typename serializable_type<std::complex<T> >::type const *> (&v);
            }
        #endif
        template<typename T, typename U> typename boost::disable_if<typename boost::mpl::or_<
            typename boost::is_same<U, T>::type,
            typename boost::is_same<U, std::complex<T> >::type
        >::type>::type set_data(std::complex<T> &, std::vector<U> const &, std::vector<hsize_t> const &, std::vector<hsize_t> const &) {
            ALPS_HDF5_THROW_RUNTIME_ERROR("invalid type conversion")
        }
        template<typename T> void set_data(std::complex<T> & v, std::vector<T> const & u, std::vector<hsize_t> const & s, std::vector<hsize_t> const & c) {
            if (s.size() != 1 || c.size() != 1 || c[0] == 0 || u.size() < c[0])
                ALPS_HDF5_THROW_RANGE_ERROR("invalid data size")
            std::copy(u.begin(), u.begin() + c[0], reinterpret_cast<T *>(&v) + s[0]);
        }
        template<typename T> void set_data(std::complex<T> & v, std::vector<std::complex<double> > const & u, std::vector<hsize_t> const & s, std::vector<hsize_t> const & c) {
            if (s.size() != 1 || c.size() != 1 || c[0] == 0 || u.size() < c[0])
                ALPS_HDF5_THROW_RANGE_ERROR("invalid data size")
            std::copy(u.begin(), u.begin() + c[0], &v + s[0]);
        }

        #define ALPS_HDF5_DEFINE_VECTOR_TYPE(C)                                                                                                            \
            template<typename T> struct serializable_type< C <T> > {                                                                                       \
                typedef typename serializable_type<typename boost::remove_const<T>::type>::type type;                                                      \
            };                                                                                                                                             \
            template<typename T> struct native_type< C <T> > { typedef typename native_type<typename boost::remove_const<T>::type>::type type; };          \
            template<typename T> struct is_cplx< C <T> > : public is_cplx<typename boost::remove_const<T>::type> { };                                      \
            template<typename T> std::vector<hsize_t> get_extent( C <T> const & v) {                                                                       \
                std::vector<hsize_t> s(1, v.size());                                                                                                       \
                if (!is_native<T>::value && v.size()) {                                                                                                    \
                    std::vector<hsize_t> t(call_get_extent(v[0]));                                                                                         \
                    std::copy(t.begin(), t.end(), std::back_inserter(s));                                                                                  \
                    for (std::size_t i = 1; i < v.size(); ++i)                                                                                             \
                        if (!std::equal(t.begin(), t.end(), call_get_extent(v[i]).begin()))                                                                \
                            ALPS_HDF5_THROW_RANGE_ERROR("no rectengual matrix")                                                                            \
                }                                                                                                                                          \
                return s;                                                                                                                                  \
            }                                                                                                                                              \
            template<typename T> void set_extent( C <T> & v, std::vector<std::size_t> const & s) {                                                         \
                if(                                                                                                                                        \
                       !(s.size() == 1 && s[0] == 0)                                                                                                       \
                    && ((is_native<T>::value && s.size() != 1) || (!is_native<T>::value && s.size() < 2))                                                  \
                )                                                                                                                                          \
                    ALPS_HDF5_THROW_RANGE_ERROR("invalid data size")                                                                                       \
                v.resize(s[0]);                                                                                                                            \
                if (!is_native<T>::value)                                                                                                                  \
                    for (std::size_t i = 0; i < s[0]; ++i)                                                                                                 \
                        call_set_extent(v[i], std::vector<std::size_t>(s.begin() + 1, s.end()));                                                           \
            }                                                                                                                                              \
            template<typename T> std::vector<hsize_t> get_offset( C <T> const & v) {                                                                       \
                if (v.size() == 0)                                                                                                                         \
                    return std::vector<hsize_t>(1, 0);                                                                                                     \
                else if (is_native<T>::value && boost::is_same<typename native_type<T>::type, std::string>::value)                                         \
                    return std::vector<hsize_t>(1, 1);                                                                                                     \
                else if (is_native<T>::value)                                                                                                              \
                    return call_get_extent(v);                                                                                                             \
                else {                                                                                                                                     \
                   std::vector<hsize_t> c(1, 1), d(call_get_offset(v[0]));                                                                                 \
                    std::copy(d.begin(), d.end(), std::back_inserter(c));                                                                                  \
                    return c;                                                                                                                              \
                }                                                                                                                                          \
            }                                                                                                                                              \
            template<typename T> bool is_vectorizable( C <T> const & v) {                                                                                  \
                for(std::size_t i = 0; i < v.size(); ++i)                                                                                                  \
                    if (!call_is_vectorizable(v[i]) || call_get_extent(v[0])[0] != call_get_extent(v[i])[0])                                               \
                        return false;                                                                                                                      \
                return true;                                                                                                                               \
            }                                                                                                                                              \
            template<typename T> typename serializable_type< C <T> >::type const * get_data(                                                               \
                  std::vector<typename serializable_type< C <T> >::type> & m                                                                               \
                ,  C <T> const & v                                                                                                                         \
                , std::vector<hsize_t> const & s                                                                                                           \
                , std::vector<hsize_t> const & = std::vector<hsize_t>()                                                                                    \
            ) {                                                                                                                                            \
                if (is_native<T>::value)                                                                                                                   \
                    return call_get_data(                                                                                                                  \
                          m                                                                                                                                \
                        , (const_cast< C <T> &>(v))[static_cast<std::size_t>(s[0])]                                                                        \
                        , std::vector<hsize_t>(s.begin() + 1, s.end())                                                                                     \
                        , call_get_extent(v)                                                                                                               \
                    );                                                                                                                                     \
                else                                                                                                                                       \
                    return call_get_data(m, (const_cast< C <T> &>(v))[static_cast<std::size_t>(s[0])], std::vector<hsize_t>(s.begin() + 1, s.end()));      \
            }                                                                                                                                              \
            template<typename T, typename U> void set_data(                                                                                                \
                C <T> & v, std::vector<U> const & u, std::vector<hsize_t> const & s, std::vector<hsize_t> const & c                                        \
            ) {                                                                                                                                            \
                if (is_native<T>::value)                                                                                                                   \
                    call_set_data(v[s[0]], u, s, c);                                                                                                       \
                else                                                                                                                                       \
                    call_set_data(v[s[0]], u, std::vector<hsize_t>(s.begin() + 1, s.end()), std::vector<hsize_t>(c.begin() + 1, c.end()));                 \
            }
        ALPS_HDF5_DEFINE_VECTOR_TYPE(std::vector)
        ALPS_HDF5_DEFINE_VECTOR_TYPE(std::valarray)
        ALPS_HDF5_DEFINE_VECTOR_TYPE(boost::numeric::ublas::vector)
        #undef ALPS_HDF5_DEFINE_VECTOR_TYPE

        template<typename T> struct serializable_type<std::pair<T *, std::vector<std::size_t> > > { typedef typename serializable_type<typename boost::remove_const<T>::type>::type type; };
        template<typename T> struct native_type<std::pair<T *, std::vector<std::size_t> > > { typedef typename native_type<typename boost::remove_const<T>::type>::type type; };
        template<typename T> struct is_cplx<std::pair<T *, std::vector<std::size_t> > > : public is_cplx<typename boost::remove_const<T>::type> { };
        template<typename T> std::vector<hsize_t> get_extent(std::pair<T *, std::vector<std::size_t> > const & v) {
            std::vector<hsize_t> s(v.second.begin(), v.second.end());
            if (!is_native<T>::value && v.second.size()) {
                 std::vector<hsize_t> t(call_get_extent(*v.first));
                 std::copy(t.begin(), t.end(), std::back_inserter(s));
                 for (std::size_t i = 1; i < std::accumulate(v.second.begin(), v.second.end(), std::size_t(1), std::multiplies<std::size_t>()); ++i)
                     if (!std::equal(t.begin(), t.end(), call_get_extent(*(v.first + i)).begin()))
                         ALPS_HDF5_THROW_RANGE_ERROR("no rectengual matrix")
            }
            return s;
        }
        template<typename T> void set_extent(std::pair<T *, std::vector<std::size_t> > & v, std::vector<std::size_t> const & s) {
            if (!(s.size() == 1 && s[0] == 0 && std::accumulate(v.second.begin(), v.second.end(), std::size_t(0)) == 0) && !std::equal(v.second.begin(), v.second.end(), s.begin()))
                ALPS_HDF5_THROW_RANGE_ERROR("invalid data size")
            if (s.size() == 1 && s[0] == 0)
                v.first = NULL;
            else if (!is_native<T>::value && s.size() > v.second.size())
                for (std::size_t i = 0; i < std::accumulate(v.second.begin(), v.second.end(), std::size_t(1), std::multiplies<hsize_t>()); ++i)
                    call_set_extent(*(v.first + i), std::vector<std::size_t>(s.begin() + v.second.size(), s.end()));
        }
        template<typename T> std::vector<hsize_t> get_offset(std::pair<T *, std::vector<std::size_t> > const & v) {
            if (is_native<T>::value && boost::is_same<typename native_type<std::pair<T *, std::vector<std::size_t> > >::type, std::string>::value)
                return std::vector<hsize_t>(v.second.size(), 1);
            else if (is_native<T>::value)
                return std::vector<hsize_t>(v.second.begin(), v.second.end());
            else {
                std::vector<hsize_t> c(v.second.size(), 1), d(call_get_offset(*v.first));
                std::copy(d.begin(), d.end(), std::back_inserter(c));
                return c;
            }
        }
        template<typename T> bool is_vectorizable(std::pair<T *, std::vector<std::size_t> > const & v) {
            for (std::size_t i = 0; i < std::accumulate(v.second.begin(), v.second.end(), std::size_t(1), std::multiplies<hsize_t>()); ++i)
                if (!call_is_vectorizable(v.first[i]) || call_get_extent(v.first[0])[0] != call_get_extent(v.first[i])[0])
                    return false;
            return true;
        }
        template<typename T> typename serializable_type<std::pair<T *, std::vector<std::size_t> > >::type const * get_data(
              std::vector<typename serializable_type<std::pair<T *, std::vector<std::size_t> > >::type> & m
            , std::pair<T *, std::vector<std::size_t> > const & v
            , std::vector<hsize_t> const & s
            , std::vector<hsize_t> const & = std::vector<hsize_t>()
        ) {
            hsize_t start = 0;
            for (std::size_t i = 0; i < v.second.size(); ++i)
                start += s[i] * std::accumulate(v.second.begin() + i + 1, v.second.end(), hsize_t(1), std::multiplies<hsize_t>());
            if (is_native<T>::value)
                return call_get_data(
                       m
                    , v.first[start]
                    , std::vector<hsize_t>(s.begin() + v.second.size(), s.end())
                    , std::vector<hsize_t>(1, std::accumulate(v.second.begin(), v.second.end(), hsize_t(1), std::multiplies<hsize_t>()))
                );
            else
                return call_get_data(
                       m
                     , v.first[start]
                     , std::vector<hsize_t>(s.begin() + v.second.size(), s.end())
                );
        }
        template<typename T, typename U> void set_data(std::pair<T *, std::vector<std::size_t> > & v, std::vector<U> const & u, std::vector<hsize_t> const & s, std::vector<hsize_t> const & c) {
            std::vector<hsize_t> start(1, 0);
            for (std::size_t i = 0; i < v.second.size(); ++i)
                start[0] += s[i] * std::accumulate(v.second.begin() + i + 1, v.second.end(), hsize_t(1), std::multiplies<hsize_t>());
            std::copy(s.begin() + v.second.size(), s.end(), std::back_inserter(start));
            if (is_native<T>::value)
                call_set_data(v.first[start[0]], u, start, std::vector<hsize_t>(1, std::accumulate(c.begin(), c.end(), hsize_t(1), std::multiplies<hsize_t>())));
            else
                call_set_data(v.first[start[0]], u, std::vector<hsize_t>(start.begin() + 1, start.end()), std::vector<hsize_t>(c.begin() + v.second.size(), c.end()));
        }

        namespace detail {
            
            struct ALPS_DECL error {
                static herr_t noop(hid_t) { return 0; }
                static herr_t callback(unsigned n, H5E_error2_t const * desc, void * buffer);
                static std::string invoke(hid_t id);
            };
            
            template<herr_t(*F)(hid_t)> class resource {
                public:
                    resource(): _id(-1) {}
                    resource(hid_t id): _id(id) {
                        if (_id < 0)
                            ALPS_HDF5_THROW_RUNTIME_ERROR(error::invoke(_id))
                    }
                    ~resource() {
                        if(_id < 0 || (_id = F(_id)) < 0) {
                            std::cerr << "Error in " << __FILE__ << " on " << ALPS_HDF5_STRINGIFY(__LINE__) << " in " << __FUNCTION__ << ":" << std::endl << error::invoke(_id) << std::endl;
                            std::abort();
                        }
                    }
                    operator hid_t() const { 
                        return _id; 
                    }
                    resource<F> & operator=(hid_t id) { 
                        if ((_id = id) < 0) 
                            ALPS_HDF5_THROW_RUNTIME_ERROR(error::invoke(_id))
                        return *this; 
                    }
                private:
                    hid_t _id;
            };
            typedef resource<H5Fclose> file_type;
            typedef resource<H5Gclose> group_type;
            typedef resource<H5Dclose> data_type;
            typedef resource<H5Aclose> attribute_type;
            typedef resource<H5Sclose> space_type;
            typedef resource<H5Tclose> type_type;
            typedef resource<H5Pclose> property_type;
            typedef resource<error::noop> error_type;
            template <typename T> T check_file(T id) { file_type unused(id); return unused; }
            template <typename T> T check_group(T id) { group_type unused(id); return unused; }
            template <typename T> T check_data(T id) { data_type unused(id); return unused; }
            template <typename T> T check_attribute(T id) { attribute_type unused(id); return unused; }
            template <typename T> T check_space(T id) { space_type unused(id); return unused; }
            template <typename T> T check_type(T id) { type_type unused(id); return unused; }
            template <typename T> T check_property(T id) { property_type unused(id); return unused; }
            template <typename T> T check_error(T id) { error_type unused(id); return unused; }
            
            struct ALPS_DECL context : boost::noncopyable {
				context(std::string const & filename, hid_t file_id, bool compress);
				~context();
				bool _compress;
				// TODO: this has to be checkd before every operation to make sure its still the same
				int _revision;
				hid_t _state_id;
				hid_t _log_id;
				hid_t _complex_id;
				std::string _filename;
				file_type _file_id;
				static bool _ignore_python_destruct_error;
            };
            #define ALPS_HDF5_FOREACH_SCALAR_NO_LONG_LONG(callback)                                                                                        \
                callback(char)                                                                                                                             \
                callback(signed char)                                                                                                                      \
                callback(unsigned char)                                                                                                                    \
                callback(short)                                                                                                                            \
                callback(unsigned short)                                                                                                                   \
                callback(int)                                                                                                                              \
                callback(unsigned int)                                                                                                                     \
                callback(long)                                                                                                                             \
                callback(unsigned long)                                                                                                                    \
                callback(float)                                                                                                                            \
                callback(double)                                                                                                                           \
                callback(long double)
            #ifndef BOOST_NO_LONG_LONG
                #define ALPS_HDF5_FOREACH_SCALAR(callback)                                                                                                 \
                    ALPS_HDF5_FOREACH_SCALAR_NO_LONG_LONG(callback)                                                                                        \
                    callback(long long)                                                                                                                    \
                    callback(unsigned long long)
            #else
                #define ALPS_HDF5_FOREACH_SCALAR(callback)                                                                                                 \
                    ALPS_HDF5_FOREACH_SCALAR_NO_LONG_LONG(callback)
            #endif
        }

        template<typename T> std::vector<hsize_t> call_get_extent(T const & v) {
            return get_extent(v);
        }
        template<typename T> void call_set_extent(T & v, std::vector<std::size_t> const & s) {
            return set_extent(v, s);
        }
        template<typename T> std::vector<hsize_t> call_get_offset(T const & v) {
            return get_offset(v);
        }
        template<typename T> bool call_is_vectorizable(T const & v) {
            return is_vectorizable(v);
        }
        template<typename T, typename U> U const * call_get_data(
              std::vector<U> & m
            , T const & v
            , std::vector<hsize_t> const & s
            , boost::optional<std::vector<hsize_t> > const & c
        ) {
            return c ? get_data(m, v, s, *c) : get_data(m, v, s);
        }
        template<typename T, typename U> void call_set_data(T & v, std::vector<U> const & u, std::vector<hsize_t> const & s, std::vector<hsize_t> const & c) {
            return set_data(v, u, s, c);
        }

         class ALPS_DECL archive_base {
            public:
                struct log_type {
                    boost::posix_time::ptime time;
                    std::string name;
                };

                archive_base(archive_base const & rhs);
                virtual ~archive_base();
                std::string const & filename() const;
                std::string encode_segment(std::string const & s);
                std::string decode_segment(std::string const & s);
                void commit(std::string const & name = "");
                std::vector<std::pair<std::string, std::string> > list_revisions() const;
                void export_revision(std::size_t revision, std::string const & file) const;
                std::string get_context() const;
                void set_context(std::string const & context);
                std::string complete_path(std::string const & p) const;
                bool is_group(std::string const & p) const;
                bool is_data(std::string const & p) const;
                bool is_attribute(std::string const & p) const;
                std::vector<std::size_t> extent(std::string const & p) const;
                std::size_t dimensions(std::string const & p) const;
                bool is_scalar(std::string const & p) const;
                bool is_string(std::string const & p) const;
                bool is_int(std::string const & p) const;
                bool is_uint(std::string const & p) const;
                bool is_long(std::string const & p) const;
                bool is_ulong(std::string const & p) const;
                bool is_longlong(std::string const & p) const;
                bool is_ulonglong(std::string const & p) const;
                bool is_float(std::string const & p) const;
                bool is_double(std::string const & p) const;
                bool is_complex(std::string const & p) const;
                bool is_null(std::string const & p) const;
                void serialize(std::string const & p);
                void delete_data(std::string const & p) const;
                void delete_group(std::string const & p) const;
                void delete_attribute(std::string const & p) const;
                std::vector<std::string> list_children(std::string const & p) const;
                std::vector<std::string> list_attributes(std::string const & p) const;
            
            protected:
                archive_base(std::string const & filename, hid_t(* F )(std::string const &), bool compress = false);
                hid_t create_path(std::string const & p, hid_t type_id, hid_t space_id, int d, hsize_t const * s = NULL, bool set_prop = true) const;
                hid_t create_dataset(std::string const & p, hid_t type_id, hid_t space_id, int d, hsize_t const * s = NULL, bool set_prop = true) const;
                void copy_attributes(hid_t dest_id, hid_t source_id, std::vector<std::string> const & names) const;
                hid_t save_comitted_data(std::string const & p, hid_t type_id, hid_t space_id, int d, hsize_t const * s = NULL, bool set_prop = true) const;
                hid_t open_attribute(std::string const & p) const;
                
                template<typename T, typename U> void get_helper_read(T & v, hid_t data_id, hid_t type_id, bool is_attr) const {
                    std::vector<hsize_t> size(call_get_extent(v)), start(size.size(), 0), count(call_get_offset(v));
                    if (
                           std::equal(count.begin(), count.end(), size.begin())
                        && H5Tget_class(type_id) == H5T_STRING && !detail::check_error(H5Tis_variable_str(type_id))
                    ) {
                        std::string data(H5Tget_size(type_id) + 1, '\0');
                        detail::check_error(is_attr ? H5Aread(data_id, type_id, &data[0]) : H5Dread(data_id, type_id, H5S_ALL, H5S_ALL, H5P_DEFAULT, &data[0]));
                        call_set_data(v, std::vector<char *>(1, &data[0]), start, count);
                    } else if (std::equal(count.begin(), count.end(), size.begin())) {
                        std::vector<U> data(std::accumulate(count.begin(), count.end(), std::size_t(1), std::multiplies<std::size_t>()));
                        detail::check_error(is_attr ? H5Aread(data_id, type_id, &data.front()) : H5Dread(data_id, type_id, H5S_ALL, H5S_ALL, H5P_DEFAULT, &data.front()));
                        call_set_data(v, data, start, count);
                        if (!is_attr && boost::is_same<U, char *>::value)
                            detail::check_error(H5Dvlen_reclaim(type_id, detail::space_type(H5Dget_space(data_id)), H5P_DEFAULT, &data.front()));
                    } else if (is_attr) {
                        std::size_t last = count.size() - 1, pos;
                        for(;count[last] == size[last]; --last);
                        std::vector<U> data(std::accumulate(size.begin(), size.end(), hsize_t(1), std::multiplies<hsize_t>()));
                        std::vector<U> chunk(std::accumulate(count.begin(), count.end(), hsize_t(1), std::multiplies<hsize_t>()));
                        detail::check_error(H5Aread(data_id, type_id, &data.front()));
                        do {
                            std::size_t sum = 0;
                            for (std::vector<hsize_t>::const_iterator it = start.begin(); it != start.end(); ++it)
                                sum += static_cast<std::size_t>(*it * std::accumulate(size.begin() + (it - start.begin()) + 1, size.end(), hsize_t(1), std::multiplies<hsize_t>()));
                            std::copy(
                                data.begin() + sum,
                                data.begin() + sum + chunk.size(),
                                chunk.begin()
                            );
                            call_set_data(v, chunk, start, count);
                            if (start[last] + 1 == size[last] && last) {
                                for (pos = last; ++start[pos] == size[pos] && pos; --pos);
                                for (++pos; pos <= last; ++pos)
                                    start[pos] = 0;
                            } else
                                ++start[last];
                        } while (start[0] < size[0]);
                    } else {
                        std::size_t last = count.size() - 1, pos;
                        for(;count[last] == size[last]; --last);
                        std::vector<U> data(std::accumulate(count.begin(), count.end(), std::size_t(1), std::multiplies<std::size_t>()));
                        do {
                            detail::space_type space_id(H5Dget_space(data_id));
                            detail::check_error(H5Sselect_hyperslab(space_id, H5S_SELECT_SET, &start.front(), NULL, &count.front(), NULL));
                            detail::space_type mem_id(H5Screate_simple(static_cast<int>(count.size()), &count.front(), NULL));
                            detail::check_error(H5Dread(data_id, type_id, mem_id, space_id, H5P_DEFAULT, &data.front()));
                            call_set_data(v, data, start, count);
                            if (boost::is_same<U, char *>::value)
                                detail::check_error(H5Dvlen_reclaim(type_id, mem_id, H5P_DEFAULT, &data.front()));
                            if (start[last] + 1 == size[last] && last) {
                                for (pos = last; ++start[pos] == size[pos] && pos; --pos);
                                for (++pos; pos <= last; ++pos)
                                    start[pos] = 0;
                            } else
                                ++start[last];
                        } while (start[0] < size[0]);
                    }
                }

                template<typename T, typename I> void get_helper(std::string const & p, T & v, bool is_attr) const {
                    if (is_scalar(p) != is_native<T>::value
                        #ifndef ALPS_HDF5_WRITE_PYTHON_COMPATIBLE_COMPLEX
                            && (!is_complex(p) || !is_cplx<T>::value)
                        #endif
                    )
                        ALPS_HDF5_THROW_RUNTIME_ERROR("scalar - vector conflict in path: " + p)
                    else if (is_native<T>::value && is_null(p))
                        ALPS_HDF5_THROW_RUNTIME_ERROR("scalars cannot be null in path: " + p)
                    else if (is_null(p))
                        call_set_extent(v, std::vector<std::size_t>(1, 0));
                    else {
                        std::vector<hsize_t> size(dimensions(p), 0);
                        I data_id(is_attr ? open_attribute(p) : H5Dopen2(file_id(), p.c_str(), H5P_DEFAULT));
                        detail::type_type type_id(is_attr ? H5Aget_type(data_id) : H5Dget_type(data_id));
                        detail::type_type native_id(H5Tget_native_type(type_id, H5T_DIR_ASCEND));
                        if (size.size()) {
                            detail::space_type space_id(is_attr ? H5Aget_space(data_id) : H5Dget_space(data_id));
                            detail::check_error(H5Sget_simple_extent_dims(space_id, &size.front(), NULL));
                        }
                        call_set_extent(v, std::vector<std::size_t>(size.begin(), size.end()));
                        if (H5Tget_class(native_id) == H5T_STRING)
                            get_helper_read<T, char *>(v, data_id, type_id, is_attr);
                        else if (detail::check_error(H5Tequal(detail::type_type(H5Tcopy(complex_id())), detail::type_type(H5Tcopy(type_id)))))
                            get_helper_read<T, std::complex<double> >(v, data_id, type_id, is_attr);
                        #define ALPS_HDF5_GET_STRING(U)                                                                                                     \
                            else if (detail::check_error(                                                                                                   \
                                H5Tequal(detail::type_type(H5Tcopy(native_id)), detail::type_type(get_native_type(static_cast<U>(0))))                      \
                            ) > 0)                                                                                                                          \
                                get_helper_read<T, U>(v, data_id, type_id, is_attr);
                        ALPS_HDF5_FOREACH_SCALAR(ALPS_HDF5_GET_STRING)
                        #undef ALPS_HDF5_GET_STRING
                        else ALPS_HDF5_THROW_RUNTIME_ERROR("invalid type")
                    }
                }

                template<typename T> void get_data(std::string const & p, T & v) const {
                    get_helper<T, detail::data_type>(p, v, false);
                }

                template<typename T> void get_attribute(std::string const & p, T & v) const {
                    get_helper<T, detail::attribute_type>(p, v, true);
                }

                template<typename T> void set_data(std::string const & p, T const & v) const {
                    if (is_group(p))
                        delete_group(p);
                    detail::type_type type_id(get_native_type(typename native_type<T>::type()));
                    std::vector<hsize_t> size(call_get_extent(v)), start(size.size(), 0), count(call_get_offset(v));
                    std::vector<typename serializable_type<T>::type> data;
                    if (is_native<T>::value) {
                        detail::data_type data_id(save_comitted_data(p, type_id, H5Screate(H5S_SCALAR), 0, NULL, !boost::is_same<typename native_type<T>::type, std::string>::value));
                        detail::check_error(H5Dwrite(data_id, type_id, H5S_ALL, H5S_ALL, H5P_DEFAULT, call_get_data(data, v, start)));
                    } else if (std::accumulate(size.begin(), size.end(), hsize_t(0)) == 0)
                        detail::check_data(save_comitted_data(p, type_id, H5Screate(H5S_NULL), 0, NULL, !boost::is_same<typename native_type<T>::type, std::string>::value));
                    else {
                        detail::data_type data_id(save_comitted_data(p, type_id, H5Screate_simple(static_cast<int>(size.size()), &size.front(), NULL), static_cast<int>(size.size()), &size.front(), !boost::is_same<typename native_type<T>::type, std::string>::value));
                        if (std::equal(count.begin(), count.end(), size.begin()))
                            detail::check_error(H5Dwrite(data_id, type_id, H5S_ALL, H5S_ALL, H5P_DEFAULT, call_get_data(data, v, start)));
                        else {
                            std::size_t last = count.size() - 1, pos;
                            for(;count[last] == size[last]; --last);
                            do {
                                detail::space_type space_id(H5Dget_space(data_id));
                                detail::check_error(H5Sselect_hyperslab(space_id, H5S_SELECT_SET, &start.front(), NULL, &count.front(), NULL));
                                detail::space_type mem_id(H5Screate_simple(static_cast<int>(count.size()), &count.front(), NULL));
                                detail::check_error(H5Dwrite(data_id, type_id, mem_id, space_id, H5P_DEFAULT, call_get_data(data, v, start)));
                                if (start[last] + 1 == size[last] && last) {
                                    for (pos = last; ++start[pos] == size[pos] && pos; --pos);
                                    for (++pos; pos <= last; ++pos)
                                        start[pos] = 0;
                                } else
                                    ++start[last];
                            } while (start[0] < size[0]);
                        }
                    }
                }
            
                template<typename I, typename O> void set_attr_copy(I i, O o, std::size_t n, std::size_t s) const {
                    std::memcpy(&*o, i, n * s);
                }
            
                void set_attr_copy(
                    serializable_type<std::string>::type const * i, 
                    std::vector<native_type<std::string>::type>::iterator o, 
                    std::size_t n, 
                    std::size_t s
                ) const {
                    std::copy(i, i + n, o);
                }
            
                template<typename T> void set_attribute(std::string const & p, T const & v) const {
                    hid_t parent_id;
                    std::string rev_path = "/revisions/" + detail::convert<std::string>(revision()) + p;
                    if (is_group(p.substr(0, p.find_last_of('@') - 1))) {
                        parent_id = detail::check_error(H5Gopen2(file_id(), p.substr(0, p.find_last_of('@') - 1).c_str(), H5P_DEFAULT));
                        if (revision() && p.substr(0, p.find_last_of('@') - 1).substr(0, std::strlen("/revisions")) != "/revisions" && !is_group(rev_path))
                            set_group(rev_path);
                    } else if (is_data(p.substr(0, p.find_last_of('@') - 1))) {
                        parent_id = detail::check_error(H5Dopen2(file_id(), p.substr(0, p.find_last_of('@') - 1).c_str(), H5P_DEFAULT));
                        if (revision() && p.substr(0, p.find_last_of('@') - 1).substr(0, std::strlen("/revisions")) != "/revisions" && !is_data(rev_path))
                            set_data(rev_path, detail::internal_state_type::PLACEHOLDER);
                    } else
                        ALPS_HDF5_THROW_RUNTIME_ERROR("unknown path: " + p.substr(0, p.find_last_of('@') - 1))
                    if (revision() 
                        && p.substr(0, p.find_last_of('@') - 1).substr(0, std::strlen("/revisions")) 
                           != "/revisions" && !detail::check_error(H5Aexists(parent_id, p.substr(p.find_last_of('@') + 1).c_str()))
                    )
                        set_attribute(rev_path + "/@" + p.substr(p.find_last_of('@') + 1), detail::internal_state_type::CREATE);
                    else if (revision() && p.substr(0, p.find_last_of('@') - 1).substr(0, std::strlen("/revisions")) != "/revisions") {
                        hid_t data_id = (is_group(rev_path) ? H5Gopen2(file_id(), rev_path.c_str(), H5P_DEFAULT) : H5Dopen2(file_id(), rev_path.c_str(), H5P_DEFAULT));
                        if (detail::check_error(H5Aexists(data_id, p.substr(p.find_last_of('@') + 1).c_str())) 
                            && detail::check_error(
                                  H5Tequal(detail::type_type(H5Aget_type(detail::attribute_type(H5Aopen(data_id, p.substr(p.find_last_of('@') + 1).c_str(), H5P_DEFAULT))))
                                , detail::type_type(H5Tcopy(state_id())))
                               ) > 0
                        )
                            H5Adelete(data_id, p.substr(p.find_last_of('@') + 1).c_str());
                        if (!detail::check_error(H5Aexists(data_id, p.substr(p.find_last_of('@') + 1).c_str())))
                            copy_attributes(data_id, parent_id, std::vector<std::string>(1, p.substr(p.find_last_of('@') + 1)));
                        if (is_group(p.substr(0, p.find_last_of('@') - 1)))
                            detail::check_group(data_id);
                        else
                            detail::check_data(data_id);
                    }
                    detail::type_type type_id(get_native_type(typename native_type<T>::type()));
                    std::vector<hsize_t> size(call_get_extent(v)), start(size.size(), 0), count(call_get_offset(v));
                    std::vector<typename serializable_type<T>::type> data;
                    hid_t id = H5Aopen(parent_id, p.substr(p.find_last_of('@') + 1).c_str(), H5P_DEFAULT);
                    if (id > 0) {
                        detail::space_type space_id(H5Aget_space(id));
                        H5S_class_t type = H5Sget_simple_extent_type(space_id);
                        if (type == H5S_NO_CLASS)
                            ALPS_HDF5_THROW_RUNTIME_ERROR("error reading class " + p)
                        if ((size.size() > 0 && size[0] > 0 && type != H5S_SCALAR)
                            || (size.size() > 0 && size[0] == 0 && type != H5S_NULL)
                            || !detail::check_error(H5Tequal(detail::type_type(H5Aget_type(id)), detail::type_type(H5Tcopy(type_id))))
                        ) {
                            detail::check_attribute(id);
                            H5Adelete(parent_id, p.substr(p.find_last_of('@') + 1).c_str());
                            id = -1;
                        }
                    }
                    if (is_native<T>::value) {
                        if (id < 0)
                            id = H5Acreate2(parent_id, p.substr(p.find_last_of('@') + 1).c_str(), type_id, detail::space_type(H5Screate(H5S_SCALAR)), H5P_DEFAULT, H5P_DEFAULT);
                        detail::check_error(H5Awrite(id, type_id, call_get_data(data, v, start)));
                    } else if (std::accumulate(size.begin(), size.end(), hsize_t(0)) == 0) {
                        if (id < 0)
                            id = H5Acreate2(parent_id, p.substr(p.find_last_of('@') + 1).c_str(), type_id, detail::space_type(H5Screate(H5S_NULL)), H5P_DEFAULT, H5P_DEFAULT);
                    } else {
                        if (id < 0)
                            id = H5Acreate2(parent_id, p.substr(p.find_last_of('@') + 1).c_str(), type_id, detail::space_type(H5Screate_simple(static_cast<int>(size.size()), &size.front(), NULL)), H5P_DEFAULT, H5P_DEFAULT);
                        if (std::equal(count.begin(), count.end(), size.begin()))
                            detail::check_error(H5Awrite(id, type_id, call_get_data(data, v, start)));
                        else {
                            std::vector<typename native_type<T>::type> continous(static_cast<std::size_t>(std::accumulate(size.begin(), size.end(), hsize_t(1), std::multiplies<hsize_t>())));
                            std::size_t last = count.size() - 1, pos;
                            for(;count[last] == size[last]; --last);
                            do {
                                std::size_t sum = 0;
                                for (std::vector<hsize_t>::const_iterator it = start.begin(); it != start.end(); ++it)
                                    sum += *it * std::accumulate(size.begin() + (it - start.begin()) + 1, size.end(), hsize_t(1), std::multiplies<hsize_t>());
                                set_attr_copy(
                                    call_get_data(data, v, start),
                                    continous.begin() + sum,
                                    std::accumulate(count.begin(), count.end(), hsize_t(1), std::multiplies<hsize_t>()),
                                    sizeof(typename native_type<T>::type)
                                );
                                if (start[last] + 1 == size[last] && last) {
                                    for (pos = last; ++start[pos] == size[pos] && pos; --pos);
                                    for (++pos; pos <= last; ++pos)
                                        start[pos] = 0;
                                } else
                                    ++start[last];
                            } while (start[0] < size[0]);
                            detail::check_error(H5Awrite(id, type_id, &continous.front()));
                        }
                    }
                    detail::attribute_type attr_id(id);
                    if (is_group(p.substr(0, p.find_last_of('@') - 1)))
                        detail::check_group(parent_id);
                    else
                        detail::check_data(parent_id);
                }
            
                void set_group(std::string const & p) const;
            private:
                template<typename T> hid_t get_native_type(T) const {
                    ALPS_HDF5_THROW_RUNTIME_ERROR(std::string("no native type passed: ") + typeid(T).name())
                }
                hid_t get_native_type(char) const { return H5Tcopy(H5T_NATIVE_CHAR); }
                hid_t get_native_type(signed char) const { return H5Tcopy(H5T_NATIVE_SCHAR); }
                hid_t get_native_type(unsigned char) const { return H5Tcopy(H5T_NATIVE_UCHAR); }
                hid_t get_native_type(short) const { return H5Tcopy(H5T_NATIVE_SHORT); }
                hid_t get_native_type(unsigned short) const { return H5Tcopy(H5T_NATIVE_USHORT); }
                hid_t get_native_type(int) const { return H5Tcopy(H5T_NATIVE_INT); }
                hid_t get_native_type(unsigned) const { return H5Tcopy(H5T_NATIVE_UINT); }
                hid_t get_native_type(long) const { return H5Tcopy(H5T_NATIVE_LONG); }
                hid_t get_native_type(unsigned long) const { return H5Tcopy(H5T_NATIVE_ULONG); }
                #ifndef BOOST_NO_LONG_LONG
                    hid_t get_native_type(long long) const { return H5Tcopy(H5T_NATIVE_LLONG); }
                    hid_t get_native_type(unsigned long long) const { return H5Tcopy(H5T_NATIVE_ULLONG); }
                #endif
                hid_t get_native_type(float) const { return H5Tcopy(H5T_NATIVE_FLOAT); }
                hid_t get_native_type(double) const { return H5Tcopy(H5T_NATIVE_DOUBLE); }
                hid_t get_native_type(long double) const { return H5Tcopy(H5T_NATIVE_LDOUBLE); }
                hid_t get_native_type(bool) const { return H5Tcopy(H5T_NATIVE_HBOOL); }
                #ifdef ALPS_HDF5_WRITE_PYTHON_COMPATIBLE_COMPLEX
                    template<typename T> hid_t get_native_type(std::complex<T>) const { return H5Tcopy(complex_id()); }
                #endif
                hid_t get_native_type(detail::internal_log_type) const { return H5Tcopy(log_id()); }
                hid_t get_native_type(std::string) const { 
                    hid_t type_id = H5Tcopy(H5T_C_S1);
                    detail::check_error(H5Tset_size(type_id, H5T_VARIABLE));
                    return type_id;
                }
                static herr_t child_visitor(hid_t, char const * n, const H5L_info_t *, void * d) {
                    reinterpret_cast<std::vector<std::string> *>(d)->push_back(n);
                    return 0;
                }
                static herr_t attr_visitor(hid_t, char const * n, const H5A_info_t *, void * d) {
                    reinterpret_cast<std::vector<std::string> *>(d)->push_back(n);
                    return 0;
                }
                template<typename T> bool is_type(std::string const & p) const {
                    try {
                        hid_t type_id;
                        if (p.find_last_of('@') != std::string::npos) {
                            detail::attribute_type attr_id(open_attribute(complete_path(p)));
                            type_id = H5Aget_type(attr_id);
                        } else {
                            detail::data_type data_id(H5Dopen2(file_id(), complete_path(p).c_str(), H5P_DEFAULT));
                            type_id = H5Dget_type(data_id);
                        }
                        detail::type_type native_id(H5Tget_native_type(type_id, H5T_DIR_ASCEND));
                        detail::check_type(type_id);
                        return detail::check_error(
                            H5Tequal(detail::type_type(H5Tcopy(native_id)), detail::type_type(get_native_type(static_cast<T>(0))))
                        ) > 0;
                    } catch (std::exception & ex) {
                        ALPS_HDF5_THROW_RUNTIME_ERROR("file: " + filename() + ", path: " + p + "\n" + ex.what());
                    }
                }
                inline int compress() const {
                    return _context->_compress;
                }
                inline int revision() const {
                    return _context->_revision;
                }
                inline hid_t state_id() const {
                    return _context->_state_id;
                }
                inline hid_t log_id() const {
                    return _context->_log_id;
                }
                inline hid_t complex_id() const {
                    return _context->_complex_id;
                }
                inline hid_t file_id() const {
                    return _context->_file_id;
                }
                std::string _path_context;
                detail::error_type _error_handler_id;
                boost::shared_ptr<detail::context> _context;
                static std::map<std::pair<std::string, bool>, boost::weak_ptr<detail::context> > _pool;
        };

        namespace detail {
            struct ALPS_DECL creator {
                static hid_t open_reading(std::string const & file);
                static hid_t open_writing(std::string const & file);
            };
        }

        class ALPS_DECL iarchive : public archive_base {
            public:
                iarchive(std::string const & file) : archive_base(file,&detail::creator::open_reading) {}
                iarchive(iarchive const & rhs) : archive_base(rhs) {}
                template<typename T> void serialize(std::string const & p, T & v) const {
                    if (p.find_last_of('@') != std::string::npos) {
                        #ifdef ALPS_HDF5_READ_GREEDY
                            if (is_attribute(p))
                        #endif
                                get_attribute(complete_path(p), v);
                    } else
                        #ifdef ALPS_HDF5_READ_GREEDY
                            if (is_data(p))
                        #endif
                                get_data(complete_path(p), v);
                }
        };

        class ALPS_DECL oarchive : public archive_base {
            public:
                oarchive(std::string const & file, bool compress = false) : archive_base(file, &detail::creator::open_writing, compress) {}
                oarchive(oarchive const & rhs) : archive_base(rhs) {}
                template<typename T> void serialize(std::string const & p, T const & v) const {
                    if (p.find_last_of('@') != std::string::npos)
                        set_attribute(complete_path(p), v);
                    else {
                        set_data(complete_path(p), v);
                        #ifndef ALPS_HDF5_WRITE_PYTHON_COMPATIBLE_COMPLEX
                            if (is_cplx<T>::value)
                                set_attribute(complete_path(p) + "/@__complex__", true);
                        #endif
                    }
                }
                void serialize(std::string const & p);
        };

        namespace detail {
            template<typename A, typename T> A & serialize_impl(A & ar, std::string const & p, T & v, boost::mpl::true_) {
                ar.serialize(p, v);
                return ar;
            }
            template<typename A, typename T> A & serialize_impl(A & ar, std::string const & p, T & v, boost::mpl::false_) {
                std::string c = ar.get_context();
                ar.set_context(ar.complete_path(p));
                v.serialize(ar);
                ar.set_context(c);
                return ar;
            }
        }

        template<typename T> iarchive & call_serialize(iarchive & ar, std::string const & p, T & v);
        template<typename T> oarchive & call_serialize(oarchive & ar, std::string const & p, T const & v);

        template<typename A, typename T> A & serialize(A & ar, std::string const & p, T & v) {
            return detail::serialize_impl(ar, p, v, is_native<T>());
        }

        template<typename T> iarchive & serialize(iarchive & ar, std::string const & p, std::complex<T> & v) {
            ar.serialize(p, v);
            return ar;
        }
        template<typename T> oarchive & serialize(oarchive & ar, std::string const & p, std::complex<T> const & v) {
            ar.serialize(p, v);
            return ar;
        }

        #define ALPS_HDF5_DEFINE_VECTOR_TYPE2(C)                                                                                                             \
            template<typename T> iarchive & serialize(iarchive & ar, std::string const & p, C <T> & v) {                                                    \
                if (ar.is_group(p)) {                                                                                                                       \
                    std::vector<std::string> children = ar.list_children(p);                                                                                \
                    v.resize(children.size());                                                                                                              \
                    for (std::vector<std::string>::const_iterator it = children.begin(); it != children.end(); ++it)                                        \
                        call_serialize(ar, p + "/" + *it, v[detail::convert<std::size_t>(*it)]);                                                            \
                } else                                                                                                                                      \
                    ar.serialize(p, v);                                                                                                                     \
                return ar;                                                                                                                                  \
            }                                                                                                                                               \
            template<typename T> oarchive & serialize(oarchive & ar, std::string const & p, C <T> const & v) {                                              \
                if (ar.is_group(p))                                                                                                                         \
                    ar.delete_group(p);                                                                                                                     \
                if (!v.size())                                                                                                                              \
                    ar.serialize(p, C <int>(0));                                                                                                            \
                else if (call_is_vectorizable(v))                                                                                                           \
                    ar.serialize(p, v);                                                                                                                     \
                else {                                                                                                                                      \
                    if (p.find_last_of('@') != std::string::npos)                                                                                           \
                        ALPS_HDF5_THROW_RUNTIME_ERROR("attributes needs to be vectorizable: " + ar.complete_path(p))                                        \
                    else if (ar.is_data(p))                                                                                                                 \
                        ar.delete_data(p);                                                                                                                  \
                    for (std::size_t i = 0; i < v.size(); ++i)                                                                                              \
                        call_serialize(ar, p + "/" + detail::convert<std::string>(i), v[i]);                                                                \
                }                                                                                                                                           \
                return ar;                                                                                                                                  \
            }
        ALPS_HDF5_DEFINE_VECTOR_TYPE2(std::vector)
        //ALPS_HDF5_DEFINE_VECTOR_TYPE2(std::valarray)
        //ALPS_HDF5_DEFINE_VECTOR_TYPE2(boost::numeric::ublas::vector)

        template<typename T> iarchive & serialize(iarchive & ar, std::string const & p, std::deque<T> & v) {
            std::vector<T> d;
            call_serialize(ar, p, d);
            v.resize(d.size());
            std::copy(d.begin(), d.end(), v.begin());
            return ar;
        }
        template<typename T> oarchive & serialize(oarchive & ar, std::string const & p, std::deque<T> const & v) {
            std::vector<T> d(v.begin(), v.end());
            call_serialize(ar, p, d);
            return ar;
        }

        template<typename T, typename U> iarchive & serialize(iarchive & ar, std::string const & p, std::pair<T, U> & v) {
            serialize(ar, p + "/first", v.first);
            serialize(ar, p + "/second", v.second);
            return ar;
        }
        template<typename T, typename U> oarchive & serialize(oarchive & ar, std::string const & p, std::pair<T, U> const & v) {
            serialize(ar, p + "/first", v.first);
            serialize(ar, p + "/second", v.second);
            return ar;
        }

        template<typename T> iarchive & serialize(iarchive & ar, std::string const & p, std::pair<T *, std::vector<std::size_t> > & v) {
            if (ar.is_group(p)) {
                std::vector<std::size_t> start(v.second.size(), 0);
                do {
                    std::size_t last = start.size() - 1, pos = 0;
                    std::string path = "";
                    for (std::vector<std::size_t>::const_iterator it = start.begin(); it != start.end(); ++it) {
                        path += "/" + detail::convert<std::string>(*it);
                        pos += *it * std::accumulate(v.second.begin() + (it - start.begin()) + 1, v.second.end(), std::size_t(1), std::multiplies<std::size_t>());
                    }
                    call_serialize(ar, p + path, v.first[pos]);
                    if (start[last] + 1 == v.second[last] && last) {
                        for (pos = last; ++start[pos] == v.second[pos] && pos; --pos);
                        for (++pos; pos <= last; ++pos)
                            start[pos] = 0;
                    } else
                        ++start[last];
                } while (start[0] < v.second[0]);
            } else
                detail::serialize_impl(ar, p, v, boost::mpl::true_());
            return ar;
        }
        template<typename T> oarchive & serialize(oarchive & ar, std::string const & p, std::pair<T *, std::vector<std::size_t> > const & v) {
            if (ar.is_group(p))
                ar.delete_group(p);
            if (!v.second.size())
               ar.serialize(p, make_pair(static_cast<int *>(NULL), v.second));
            else if (call_is_vectorizable(v))
                detail::serialize_impl(ar, p, v, boost::mpl::true_());
            else {
                if (p.find_last_of('@') != std::string::npos)
                    ALPS_HDF5_THROW_RUNTIME_ERROR("attributes needs to be vectorizable: " + ar.complete_path(p))
                if (ar.is_data(p))
                    ar.delete_data(p);
                std::vector<std::size_t> start(v.second.size(), 0);
                do {
                    std::size_t last = start.size() - 1, pos = 0;
                    std::string path = "";
                    for (std::vector<std::size_t>::const_iterator it = start.begin(); it != start.end(); ++it) {
                        path += "/" + detail::convert<std::string>(*it);
                        pos += *it * std::accumulate(v.second.begin() + (it - start.begin()) + 1, v.second.end(), std::size_t(1), std::multiplies<std::size_t>());
                    }
                    call_serialize(ar, p + path, v.first[pos]);
                    if (start[last] + 1 == v.second[last] && last) {
                        for (pos = last; ++start[pos] == v.second[pos] && pos; --pos);
                        for (++pos; pos <= last; ++pos)
                            start[pos] = 0;
                    } else
                        ++start[last];
                } while (start[0] < v.second[0]);
            }
            return ar;
        }

        template<typename T, std::size_t N, typename A> iarchive & serialize(iarchive & ar, std::string const & p, boost::multi_array<T, N, A> & v) {
            std::pair<T *, std::vector<std::size_t> > d(v.data(), std::vector<std::size_t>(v.shape(), v.shape() + boost::multi_array<T, N, A>::dimensionality));
            call_serialize(ar, p, d);
            return ar;
        }
        template<typename T, std::size_t N, typename A> oarchive & serialize(oarchive & ar, std::string const & p, boost::multi_array<T, N, A> const & v) {
            std::pair<T const *, std::vector<std::size_t> > d(v.data(), std::vector<std::size_t>(v.shape(), v.shape() + boost::multi_array<T, N, A>::dimensionality));
            call_serialize(ar, p, d);
            return ar;
        }

        template<typename T> iarchive & call_serialize(iarchive & ar, std::string const & p, T & v) { 
            return serialize(ar, p, v); 
        }
        template<typename T> oarchive & call_serialize(oarchive & ar, std::string const & p, T const & v) { 
            return serialize(ar, p, v); 
        }
        template <typename T> class pvp {
            public:
                pvp(std::string const & p, T v): _p(p), _v(v) {}
                pvp(pvp<T> const & c): _p(c._p), _v(c._v) {}
                template<typename A> A & serialize(A & ar) const {
                    try {
                        return call_serialize(ar, _p, const_cast<typename boost::add_reference<T>::type>(_v));
                    } catch (std::exception & ex) {
                        throw std::runtime_error("HDF5 Error " + std::string(boost::is_same<A, iarchive>::value ? "reading" : "writing") + " path '" + _p + "' on type '" + typeid(_v).name() + "':\n" + ex.what());
                    }
                }
            private:
                std::string _p;
                T _v;
        };

		#ifdef ALPS_USE_NGS

			template <typename T> oarchive & operator<< (oarchive & ar, ::alps::detail::make_pvp_proxy<T> proxy) {
				pvp<T>(proxy.path_, proxy.value_).serialize(ar);
			}
			
			template <typename T> iarchive & operator>> (iarchive & ar, ::alps::detail::make_pvp_proxy<T> proxy) {
				pvp<T>(proxy.path_, proxy.value_).serialize(ar);
			}

		#else

			template <typename T> iarchive & operator>> (iarchive & ar, pvp<T> const & v) { return v.serialize(ar); }
			template <typename T> oarchive & operator<< (oarchive & ar, pvp<T> const & v) { return v.serialize(ar); }
			
		#endif
    }
	
	#ifndef ALPS_USE_NGS

		template <typename T> typename boost::disable_if<typename boost::mpl::and_<
			  typename boost::is_same<typename boost::remove_cv<typename boost::remove_all_extents<T>::type>::type, char>::type
			, typename boost::is_array<T>::type
		>::type, hdf5::pvp<T &> >::type make_pvp(std::string const & p, T & v) {
			return hdf5::pvp<T &>(p, v);
		}
		template <typename T> typename boost::disable_if<typename boost::mpl::and_<
			  typename boost::is_same<typename boost::remove_cv<typename boost::remove_all_extents<T>::type>::type, char>::type
			, typename boost::is_array<T>::type
		>::type, hdf5::pvp<T const &> >::type make_pvp(std::string const & p, T const & v) {
			return hdf5::pvp<T const &>(p, v);
		}
		template <typename T> typename boost::enable_if<typename boost::mpl::and_<
			  typename boost::is_same<typename boost::remove_cv<typename boost::remove_all_extents<T>::type>::type, char>::type
			, typename boost::is_array<T>::type
		>::type, hdf5::pvp<std::string const> >::type make_pvp(std::string const & p, T const & v) {
			return hdf5::pvp<std::string const>(p, v);
		}

		template <typename T> hdf5::pvp<std::pair<T *, std::vector<std::size_t> > > make_pvp(std::string const & p, T * v, std::size_t s) {
			return hdf5::pvp<std::pair<T *, std::vector<std::size_t> > >(p, std::make_pair(boost::ref(v), std::vector<std::size_t>(1, s)));
		}
		template <typename T> hdf5::pvp<std::pair<T *, std::vector<std::size_t> > > make_pvp(std::string const & p, T * v, std::vector<std::size_t> const & s) {
			return hdf5::pvp<std::pair<T *, std::vector<std::size_t> > >(p, std::make_pair(boost::ref(v), s));
		}
	
	#endif

}
#endif