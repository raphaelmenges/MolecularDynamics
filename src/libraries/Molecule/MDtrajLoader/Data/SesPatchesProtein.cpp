#include "SesPatchesProtein.h"
#include "Atom.h"
#include <iostream>


void SESPatchesProtein::recenter()
{
    glm::vec3 sum = glm::vec3(0.0f);
    for (auto const& atom : atoms_) sum += atom->getPosition();
    sum /= atoms_.size();
    for (auto & atom : atoms_) atom->setXYZ(atom->getPosition() - sum);
}



void SESPatchesProtein::calculatePatches(float probeRadius)
{

    int T = 0; // timestep

    std::cout << "atom count:\t" << atoms_.size() << std::endl;

    frames[T].spherePatches.clear();

    for (std::size_t i = 0; i < atoms_.size() - 2; i++)
    {
        for (std::size_t j = i + 1; j < atoms_.size() - 1; j++)
        {
            for (std::size_t k = j + 1; k < atoms_.size(); k++)
            {
                glm::vec3 atom1_pos = atoms_[i]->getPosition();
                float     atom1_r   = this->getRadiusAt(i) + probeRadius;
                float     atom1_rr  = atom1_r * atom1_r;
                glm::vec3 atom2_pos = atoms_[j]->getPosition();
                float     atom2_r   = this->getRadiusAt(j) + probeRadius;
                float     atom2_rr  = atom2_r * atom2_r;
                glm::vec3 atom3_pos = atoms_[k]->getPosition();
                float     atom3_r   = this->getRadiusAt(k) + probeRadius;
                float     atom3_rr  = atom3_r * atom3_r;

                float     d_a1_a2   = glm::distance(atom1_pos, atom2_pos);
                float     d_a1_a3   = glm::distance(atom1_pos, atom3_pos);
                float     d_a2_a3   = glm::distance(atom2_pos, atom3_pos);


                if (d_a1_a2 < atom1_r + atom2_r && d_a1_a2 + atom1_r > atom2_r && d_a1_a2 + atom2_r > atom1_r &&
                        d_a1_a3 < atom1_r + atom3_r && d_a1_a3 + atom1_r > atom3_r && d_a1_a3 + atom3_r > atom1_r &&
                        d_a2_a3 < atom2_r + atom3_r && d_a2_a3 + atom2_r > atom3_r && d_a2_a3 + atom3_r > atom2_r)
                {
                    glm::vec3 omega   = atom1_pos;
                    glm::vec3 omega_x = glm::normalize(atom2_pos - atom1_pos);
                    float     i       = glm::dot(omega_x, atom3_pos - atom1_pos);
                    glm::vec3 omega_y = glm::normalize(atom3_pos - atom1_pos - i * omega_x);
                    glm::vec3 omega_z = glm::normalize(glm::cross(omega_x, omega_y));
                    float     d       = glm::length(atom2_pos - atom1_pos);
                    float     j       = glm::dot(omega_y, atom3_pos - atom1_pos);

                    float     x = (atom1_rr - atom2_rr + d * d) / (2 * d);

                    float     radius_help_sphere = glm::sqrt(glm::abs(atom1_rr - x * x));
                    float     distance_help_sphere_i = i - x;

                    if (glm::abs(distance_help_sphere_i) < atom3_r)
                    {
                        float     atom3_r_new = glm::sqrt(glm::abs(atom3_rr - distance_help_sphere_i * distance_help_sphere_i));

                        if (radius_help_sphere + atom3_r_new > j && radius_help_sphere + j > atom3_r_new && atom3_r_new + j > radius_help_sphere)
                        {
                            float y = (radius_help_sphere * radius_help_sphere - atom3_r_new * atom3_r_new + j * j) / (2 * j);
                            float z_abs = glm::sqrt(glm::abs(radius_help_sphere * radius_help_sphere - y * y));

                            glm::vec3 probe1_pos = omega + x * omega_x + y * omega_y + z_abs * omega_z;
                            glm::vec3 probe2_pos = omega + x * omega_x + y * omega_y - z_abs * omega_z;

                            SpherePatch sp1;
                            sp1.probe_position = probe1_pos;
                            sp1.atom1_position = atom1_pos;
                            sp1.atom2_position = atom2_pos;
                            sp1.atom3_position = atom3_pos;
                            frames[T].spherePatches.push_back(sp1);

                            SpherePatch sp2;
                            sp2.probe_position = probe2_pos;
                            sp2.atom1_position = atom2_pos; // atom1 and atom2 are switched to keep the same winding order inside the geometry shader
                            sp2.atom2_position = atom1_pos;
                            sp2.atom3_position = atom3_pos;
                            frames[T].spherePatches.push_back(sp2);


                            float d1 =glm::length(probe1_pos - atom1_pos);
                            float d2 =glm::length(probe1_pos - atom2_pos);
                            float d3 =glm::length(probe1_pos - atom3_pos);

                            if (glm::abs(d1 - atom1_r) > 0.001f || glm::abs(d2 - atom2_r) > 0.001f || glm::abs(d3 - atom3_r) > 0.001f)
                            {
                                //std::cout << "atom id: " << i << j << k << std::endl;
                                std::cout << "new" << std::endl;
                                std::cout << "\t" << "dist all: " << glm::abs(d1 - atom1_r) << "  " <<  glm::abs(d2 - atom2_r) << "  " <<  glm::abs(d3 - atom3_r)<< std::endl;
                                std::cout << "\t" << "dist a1: " << d1 << "  " <<  atom1_r << std::endl;
                                std::cout << "\t" << "dist a2: " << d2 << "  " <<  atom2_r << std::endl;
                                std::cout << "\t" << "dist a3: " << d3 << "  " <<  atom3_r << std::endl;
                                /*
                                std::cout << "\t" << "a1: " << atom1_pos.x << "\t" << atom1_pos.y << "\t" << atom1_pos.z << "\t" << atom1_r << std::endl;
                                std::cout << "\t" << "a2: " << atom2_pos.x << "\t" << atom2_pos.y << "\t" << atom2_pos.z << "\t" << atom2_r << std::endl;
                                std::cout << "\t" << "a3: " << atom3_pos.x << "\t" << atom3_pos.y << "\t" << atom3_pos.z << "\t" << atom3_r << std::endl;

                                std::cout << "\t" << "v_a1_a2:\t" << v_a1_a2.x << "\t" << v_a1_a2.y << "\t" << v_a1_a2.z << std::endl;
                                std::cout << "\t" << "d_a1_a2:\t" << d_a1_a2 << std::endl;
                                std::cout << "\t" << "d_a1_i1:\t" << d_a1_i1 << std::endl;
                                std::cout << "\t" << "r_i1:\t" << r_i1 << std::endl;
                                std::cout << "\t" << "omega_x:\t" << omega_x.x << "\t" << omega_x.y << "\t" << omega_x.z << std::endl;
                                std::cout << "\t" << "d_a1_o:\t" << d_a1_o << std::endl;
                                std::cout << "\t" << "v_a1_o:\t" << v_a1_o.x << "\t" << v_a1_o.y << "\t" << v_a1_o.z << std::endl;
                                std::cout << "\t" << "omega:\t" << omega.x << "\t" << omega.y << "\t" << omega.z << std::endl;
                                std::cout << "\t" << "v_o_a3:\t" << v_o_a3.x << "\t" << v_o_a3.y << "\t" << v_o_a3.z << std::endl;
                                std::cout << "\t" << "d_o_a3:\t" << d_o_a3 << std::endl;
                                std::cout << "\t" << "omega_y:\t" << omega_y.x << "\t" << omega_y.y << "\t" << omega_y.z << std::endl;
                                std::cout << "\t" << "omega_z:\t" << omega_z.x << "\t" << omega_z.y << "\t" << omega_z.z << std::endl;
                                std::cout << "\t" << "d_i1_o:\t" << d_i1_o << std::endl;
                                std::cout << "\t" << "r_a3_i1:\t" << r_a3_i1 << std::endl;
                                std::cout << "\t" << "d_i1_i2:\t" << d_i1_i2 << std::endl;
                                std::cout << "\t" << "r_i2:\t" << r_i2 << std::endl;
    */
                            }

                            //std::cout << "\t" << "p1: " << probe1_pos.x << "\t" << probe1_pos.y << "\t" << probe1_pos.z << std::endl;
                            //std::cout << "\t" << "p2: " << probe2_pos.x << "\t" << probe2_pos.y << "\t" << probe2_pos.z << std::endl;
                            //std::cout << "\t" << "dist: " << glm::length(probe2_pos - atom1_pos) << "\t" <<  glm::length(probe2_pos - atom2_pos) << "\t" << glm::length(probe2_pos - atom3_pos) << std::endl;
                        }
                    }
                }
            }
        }
    }

    std::cout << "sphere patch count:\t" << frames[T].spherePatches.size() << std::endl;


    frames[T].toroidalPatches.clear();

    for (std::size_t i = 0; i < atoms_.size() - 1; i++)
    {
        for (std::size_t j = i + 1; j < atoms_.size(); j++)
        {
            glm::vec3 atom1_pos = atoms_[i]->getPosition();
            float     atom1_r   = this->getRadiusAt(i) + probeRadius;
            float     atom1_rr  = atom1_r * atom1_r;
            glm::vec3 atom2_pos = atoms_[j]->getPosition();
            float     atom2_r   = this->getRadiusAt(j) + probeRadius;
            float     atom2_rr  = atom2_r * atom2_r;

            /*
            std::cout << "i: " << i << " j: " << j << std::endl;

            std::cout << "\t" << "a1: " << atom1_pos.x << "\t" << atom1_pos.y << "\t" << atom1_pos.z << std::endl;
            std::cout << "\t" << "a2: " << atom2_pos.x << "\t" << atom2_pos.y << "\t" << atom2_pos.z << std::endl;
            std::cout << "\t" << "a: " << glm::distance(atom1_pos, atom2_pos) << "\t" << atom1_r << "\t" << atom2_r << "\t" << std::endl;
*/

            if (glm::distance(atom1_pos, atom2_pos) < atom1_r + atom2_r)
            {
                //std::cout << "YES!   i: " << i << " j: " << j << std::endl;

                glm::vec3 v_a1_a2   = atom2_pos - atom1_pos;
                glm::vec3 v_a1_a2_n = glm::normalize(v_a1_a2);
                float     d_a1_a2   = glm::length(v_a1_a2);
                float     d_a1_i    = (d_a1_a2 * d_a1_a2 - atom2_rr + atom1_rr) / (2 * d_a1_a2);
                float     r_i       = glm::sqrt(glm::abs(atom1_rr - d_a1_i * d_a1_i));

                glm::vec3 v_u;
                if (v_a1_a2_n.z != 0 && v_a1_a2_n.y != 0)
                {
                    v_u             = glm::normalize(glm::vec3(0.0f, -v_a1_a2_n.z, v_a1_a2_n.y));
                }
                else
                {
                    v_u             = glm::normalize(glm::vec3(v_a1_a2_n.y, -v_a1_a2_n.x, 0.0f));
                }

                glm::vec3 v_v       = glm::normalize(glm::cross(v_a1_a2_n, v_u));
                glm::vec3 p_i_rand  = atom1_pos + d_a1_i * v_a1_a2_n + r_i * v_u;

                glm::vec3 v_a1_i    = p_i_rand - atom1_pos;
                glm::vec3 v_a1_i_n  = glm::normalize(v_a1_i);

                glm::vec3 v_a1_t1   = this->getRadiusAt(i) * v_a1_i_n;
                glm::vec3 p_t1c     = atom1_pos + glm::dot(v_a1_t1, v_a1_a2_n) * v_a1_a2_n;
                float     d_a1_t1c  = glm::length(p_t1c - atom1_pos);
                glm::vec3 v_a2_i    = p_i_rand - atom2_pos;
                glm::vec3 v_a2_i_n  = glm::normalize(v_a2_i);
                glm::vec3 v_a2_t2   = this->getRadiusAt(j) * v_a2_i_n;
                glm::vec3 p_t2c     = atom2_pos + glm::dot(v_a2_t2, -v_a1_a2_n) * -v_a1_a2_n;
                float     d_a2_t2c  = glm::length(p_t2c - atom2_pos);

                /*
                std::cout << "\t" << "v_a1_a2:\t" << v_a1_a2.x << "\t" << v_a1_a2.y << "\t" << v_a1_a2.z << std::endl;
                std::cout << "\t" << "v_a1_a2_n:\t" << v_a1_a2_n.x << "\t" << v_a1_a2_n.y << "\t" << v_a1_a2_n.z << std::endl;
                std::cout << "\t" << "d_a1_a2:\t" << d_a1_a2 << std::endl;
                std::cout << "\t" << "d_a1_i:\t" << d_a1_i << std::endl;
                std::cout << "\t" << "r_i:\t" << r_i << std::endl;
                std::cout << "\t" << "v_u:\t" << v_u.x << "\t" << v_u.y << "\t" << v_u.z << std::endl;
                std::cout << "\t" << "v_v:\t" << v_v.x << "\t" << v_v.y << "\t" << v_v.z << std::endl;
                std::cout << "\t" << "p_i_rand:\t" << p_i_rand.x << "\t" << p_i_rand.y << "\t" << p_i_rand.z << std::endl;
                std::cout << "\t" << "v_a1_i:\t" << v_a1_i.x << "\t" << v_a1_i.y << "\t" << v_a1_i.z << std::endl;
                std::cout << "\t" << "v_a1_i_n:\t" << v_a1_i_n.x << "\t" << v_a1_i_n.y << "\t" << v_a1_i_n.z << std::endl;
                std::cout << "\t" << "v_a1_t1:\t" << v_a1_t1.x << "\t" << v_a1_t1.y << "\t" << v_a1_t1.z << std::endl;
                std::cout << "\t" << "p_t1c:\t" << p_t1c.x << "\t" << p_t1c.y << "\t" << p_t1c.z << std::endl;
                std::cout << "\t" << "d_a1_t1c:\t" << d_a1_t1c << std::endl;
                std::cout << "\t" << "v_a2_i:\t" << v_a2_i.x << "\t" << v_a2_i.y << "\t" << v_a2_i.z << std::endl;
                std::cout << "\t" << "v_a2_i_n:\t" << v_a2_i_n.x << "\t" << v_a2_i_n.y << "\t" << v_a2_i_n.z << std::endl;
                std::cout << "\t" << "v_a2_t2:\t" << v_a2_t2.x << "\t" << v_a2_t2.y << "\t" << v_a2_t2.z << std::endl;
                std::cout << "\t" << "p_t2c:\t" << p_t2c.x << "\t" << p_t2c.y << "\t" << p_t2c.z << std::endl;
                std::cout << "\t" << "d_a2_t2c:\t" << d_a2_t2c << std::endl;
                */

                ToroidalPatch tp;
                tp.torus_center     = atom1_pos + d_a1_i * v_a1_a2_n;
                tp.torus_radius     = r_i;
                tp.tangent1_center = p_t1c;
                tp.tangent1_radius = glm::sqrt(this->getRadiusAt(i) * this->getRadiusAt(i) - d_a1_t1c * d_a1_t1c);
                tp.tangent2_center = p_t2c;
                tp.tangent2_radius = glm::sqrt(this->getRadiusAt(j) * this->getRadiusAt(j) - d_a2_t2c * d_a2_t2c);
                frames[T].toroidalPatches.push_back(tp);

                /*
                std::cout << "\t" << "torus_center: " << tp.torus_center.x << "\t" << tp.torus_center.y << "\t" << tp.torus_center.z << std::endl;
                std::cout << "\t" << "torus_radius: " << tp.torus_radius << std::endl;
                std::cout << "\t" << "tangent1_center: " << tp.tangent1_center.x << "\t" << tp.tangent1_center.y << "\t" << tp.tangent1_center.z << std::endl;
                std::cout << "\t" << "tangent1_radius: " << tp.tangent1_radius << std::endl;
                std::cout << "\t" << "tangent2_center: " << tp.tangent2_center.x << "\t" << tp.tangent2_center.y << "\t" << tp.tangent2_center.z << std::endl;
                std::cout << "\t" << "tangent2_radius: " << tp.tangent2_radius << std::endl;
                */
            }
        }
    }

    std::cout << "toroidal patch count:\t" << frames[T].toroidalPatches.size() << std::endl;
}
