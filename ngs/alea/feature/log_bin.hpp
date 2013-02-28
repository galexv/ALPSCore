/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                                 *
 * ALPS Project: Algorithms and Libraries for Physics Simulations                  *
 *                                                                                 *
 * ALPS Libraries                                                                  *
 *                                                                                 *
 * Copyright (C) 2011 - 2012 by Mario Koenz <mkoenz@ethz.ch>                       *
 *                                                                                 *
 * This software is part of the ALPS libraries, published under the ALPS           *
 * Library License; you can use, redistribute it and/or modify it under            *
 * the terms of the license, either version 1 or (at your option) any later        *
 * version.                                                                        *
 *                                                                                 *
 * You should have received a copy of the ALPS Library License along with          *
 * the ALPS Libraries; see the file LICENSE.txt. If not, the license is also       *
 * available from http://alps.comp-phys.org/.                                      *
 *                                                                                 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR     *
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,        *
 * FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT       *
 * SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE       *
 * FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,     *
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER     *
 * DEALINGS IN THE SOFTWARE.                                                       *
 *                                                                                 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */


#ifndef ALPS_NGS_ALEA_DETAIL_LOG_BIN_IMPLEMENTATION_HEADER
#define ALPS_NGS_ALEA_DETAIL_LOG_BIN_IMPLEMENTATION_HEADER

#include <alps/ngs/alea/feature/mean.hpp>
#include <alps/ngs/alea/feature/feature_traits.hpp>

#include <boost/cstdint.hpp>

#include <vector>
#include <ostream>
#include <cmath>
#include <algorithm>

namespace alps
{
    namespace accumulator
    {
        //=================== log_bin proxy ===================
        template<typename value_type>
        class log_bin_proxy_type
        {
            typedef typename mean_type<value_type>::type mean_type;
            typedef typename std::vector<value_type>::size_type size_type;
        public:
            log_bin_proxy_type(): bin_(std::vector<mean_type>()) {}
            log_bin_proxy_type(std::vector<mean_type> const & bin): bin_(bin) {}
            
            inline std::vector<mean_type> const & bins() const 
            {
                return bin_;
            }
            
            template<typename T>
            friend std::ostream & operator<<(std::ostream & os, log_bin_proxy_type<T> const & arg);
        private:
            std::vector<mean_type> const & bin_;
        };

        //~ template<typename value_type>
        //~ log_bin_proxy_type<value_type> make_log_bin_proxy_type()
        //~ {
            //~ return log_bin_proxy_type<value_type>(unused);
        //~ }
        
        template<typename T>
        inline std::ostream & operator<<(std::ostream & os, log_bin_proxy_type<T> const & arg)
        {
            os << "log_bin_proxy" << std::endl;
            return os;
            
        };
        //=================== log_bin trait ===================
        template <typename T>
        struct log_bin_type
        {
            typedef log_bin_proxy_type<T> type;
        };
        //=================== log_bin implementation ===================
        namespace detail
        {
            //set up the dependencies for the tag::log_binning-Implementation
            template<> 
            struct Dependencies<tag::log_binning> 
            {
                typedef MakeList<tag::mean, tag::error>::type type;
            };

            template<typename base_type> 
            class AccumulatorImplementation<tag::log_binning, base_type> : public base_type 
            {
                typedef typename base_type::value_type value_type_loc;
                typedef typename log_bin_type<value_type_loc>::type log_bin_type;
                typedef typename std::vector<value_type_loc>::size_type size_type;
                typedef typename mean_type<value_type_loc>::type mean_type;
                typedef AccumulatorImplementation<tag::log_binning, base_type> ThisType;
          
                public:    
                    AccumulatorImplementation<tag::log_binning, base_type>(ThisType const & arg):  base_type(arg)
                                                                    , bin_(arg.bin_)
                                                                    , partial_(arg.partial_)
                                                                    , pos_in_partial_(arg.pos_in_partial_)
                                                                    , bin_size_now_(arg.bin_size_now_) 
                    {}
                    
                    template<typename ArgumentPack>
                    AccumulatorImplementation<tag::log_binning, base_type>(ArgumentPack const & args
                                         , typename boost::disable_if<
                                                                      boost::is_base_of<ThisType, ArgumentPack>
                                                                    , int
                                                                    >::type = 0
                                        ): base_type(args)
                                         , partial_()
                                         , pos_in_partial_()
                                         , bin_size_now_(1)
                    {}
                    
                    inline log_bin_type const log_bin() const 
                    { 
                        return log_bin_proxy_type<value_type_loc>(bin_);
                    }
              
                    inline ThisType& operator <<(value_type_loc val) 
                    {
                        using namespace alps::ngs::numeric;
                        
                        base_type::operator <<(val);
                        
                        partial_ += val;
                        ++pos_in_partial_;
                        
                        if(pos_in_partial_ == bin_size_now_)
                        {
                            bin_.push_back(partial_ / bin_size_now_);
                            partial_ = value_type_loc();
                            pos_in_partial_ = 0;
                            bin_size_now_ *= 2;
                        }
                        return *this;
                    }
              
                    template<typename Stream>
                    inline void print(Stream & os) 
                    {
                        base_type::print(os);
                        os << "Log Binning: " << std::endl;
                        
                        //~ os << std::endl;
                        //~ for (unsigned int i = 0; i < bin_.size(); ++i)
                        //~ {
                            //~ os << "bin[" << i << "] = " << bin_[i] << std::endl;
                        //~ }
                    }
                    inline void reset()
                    {
                        base_type::reset();
                        bin_.clear();
                        partial_ = value_type_loc();
                        pos_in_partial_ = 0;
                        bin_size_now_ = 1;
                    }
                private:
                    std::vector<mean_type> bin_;
                    value_type_loc partial_;
                    size_type pos_in_partial_;
                    size_type bin_size_now_;
            };

            template<typename base_type> class ResultImplementation<tag::log_binning, base_type> {
// TODO: implement!
            };

        } // end namespace detail
    }//end accumulator namespace 
}//end alps namespace
#endif // ALPS_NGS_ALEA_DETAIL_LOG_BIN_IMPLEMENTATION
