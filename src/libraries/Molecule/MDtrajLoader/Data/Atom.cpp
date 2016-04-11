// Fill out your copyright notice in the Description page of Project Settings.

#include "Atom.h"

Atom::Atom(std::string name, std::string element, int index, glm::vec3 position, std::string aminoDistinctAcid, std::string amino, Protein* parent) :
name_(name), element_(element), index_(index), aminoDistinctAcid_(aminoDistinctAcid), amino_(amino), pParent_(parent)
{
    positions_.push_back(position);
}

Atom::~Atom()
{
}

int Atom::getIndex() {
    return index_;
}

std::string Atom::getName() {
    return name_;
}

std::string Atom::getElement() const
{
    return element_;
}

std::string Atom::getAmino() {
    return amino_;
}

std::string Atom::getDistinctResidue() {
    return aminoDistinctAcid_;
}

glm::vec3 Atom::getPosition() const{
    return positions_.at(0);
}

glm::vec3 Atom::getPositionAtFrame(int i) {
    return positions_.at(i);
}

float Atom::getX() {
    return positions_.at(0).x;
}

float Atom::getY() {
    return positions_.at(0).y;
}

float Atom::getZ() {
    return positions_.at(0).z;
}

float Atom::getXAtFrame(int i) {
    return positions_.at(i).x;
}

float Atom::getYAtFrame(int i) {
    return positions_.at(i).y;
}

float Atom::getZAtFrame(int i) {
    return positions_.at(i).z;
}

void Atom::setNextPosition(glm::vec3 pos) {
    positions_.push_back(pos);
}

void Atom::addBondPartner(Atom* partner)
{
    bonds_.push_back(partner);
}

std::vector<Atom*> Atom::getBondPartners()
{
    return bonds_;
}

void Atom::setX(float x)
{
    positions_.at(0).x = x;
}

void Atom::setY(float y)
{
    positions_.at(0).y = y;
}

void Atom::setZ(float z)
{
    positions_.at(0).z = z;
}

void Atom::setXYZ(glm::vec3 xyz)
{
    positions_.at(0) = xyz;
}

void Atom::setXYZat(int frame, glm::vec3 xyz)
{
    positions_.at(frame) = xyz;
}

Protein* Atom::getProteinParent() {
    return pParent_;
}
