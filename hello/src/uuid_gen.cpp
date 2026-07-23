//
// Created by raaveinm on 6/25/26.
//

#include <iostream>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

int main() {
    boost::uuids::random_generator gen;
    const boost::uuids::uuid id = gen();
    std::cout << "UUID: " << id << '\n' << sizeof(id) << '\n';

    const boost::uuids::name_generator_sha1 name_gen(boost::uuids::ns::dns());
    const boost::uuids::uuid dns_id = name_gen("a.com");
    std::cout << "Name-based UUID: " << dns_id << '\n';

    const boost::uuids::uuid dns_id_2 = name_gen("a.com");
    std::cout << "Name-based UUID: " << dns_id_2 << '\n';

}
