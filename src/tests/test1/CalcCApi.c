/* This file was autogenerated by cloop - Cross Language Object Oriented Programming */

#include "CalcCApi.h"


CLOOP_EXTERN_C void CALC_IDisposable_dispose(struct CALC_IDisposable* self)
{
	self->vtable->dispose(self);
}

CLOOP_EXTERN_C void CALC_IStatus_dispose(struct CALC_IStatus* self)
{
	self->vtable->dispose(self);
}

CLOOP_EXTERN_C int CALC_IStatus_getCode(const struct CALC_IStatus* self)
{
	return self->vtable->getCode(self);
}

CLOOP_EXTERN_C void CALC_IStatus_setCode(struct CALC_IStatus* self, int code)
{
	self->vtable->setCode(self, code);
}

CLOOP_EXTERN_C void CALC_IStatusFactory_dispose(struct CALC_IStatusFactory* self)
{
	self->vtable->dispose(self);
}

CLOOP_EXTERN_C struct CALC_IStatus* CALC_IStatusFactory_createStatus(struct CALC_IStatusFactory* self)
{
	return self->vtable->createStatus(self);
}

CLOOP_EXTERN_C void CALC_IFactory_dispose(struct CALC_IFactory* self)
{
	self->vtable->dispose(self);
}

CLOOP_EXTERN_C struct CALC_IStatus* CALC_IFactory_createStatus(struct CALC_IFactory* self)
{
	return self->vtable->createStatus(self);
}

CLOOP_EXTERN_C struct CALC_ICalculator* CALC_IFactory_createCalculator(struct CALC_IFactory* self, struct CALC_IStatus* status)
{
	return self->vtable->createCalculator(self, status);
}

CLOOP_EXTERN_C struct CALC_ICalculator2* CALC_IFactory_createCalculator2(struct CALC_IFactory* self, struct CALC_IStatus* status)
{
	return self->vtable->createCalculator2(self, status);
}

CLOOP_EXTERN_C struct CALC_ICalculator* CALC_IFactory_createBrokenCalculator(struct CALC_IFactory* self, struct CALC_IStatus* status)
{
	return self->vtable->createBrokenCalculator(self, status);
}

CLOOP_EXTERN_C void CALC_IFactory_setStatusFactory(struct CALC_IFactory* self, struct CALC_IStatusFactory* statusFactory)
{
	self->vtable->setStatusFactory(self, statusFactory);
}

CLOOP_EXTERN_C void CALC_ICalculator_dispose(struct CALC_ICalculator* self)
{
	self->vtable->dispose(self);
}

CLOOP_EXTERN_C int CALC_ICalculator_sum(const struct CALC_ICalculator* self, struct CALC_IStatus* status, int n1, int n2)
{
	return self->vtable->sum(self, status, n1, n2);
}

CLOOP_EXTERN_C int CALC_ICalculator_getMemory(const struct CALC_ICalculator* self)
{
	return self->vtable->getMemory(self);
}

CLOOP_EXTERN_C void CALC_ICalculator_setMemory(struct CALC_ICalculator* self, int n)
{
	self->vtable->setMemory(self, n);
}

CLOOP_EXTERN_C void CALC_ICalculator_sumAndStore(struct CALC_ICalculator* self, struct CALC_IStatus* status, int n1, int n2)
{
	self->vtable->sumAndStore(self, status, n1, n2);
}

CLOOP_EXTERN_C void CALC_ICalculator2_dispose(struct CALC_ICalculator2* self)
{
	self->vtable->dispose(self);
}

CLOOP_EXTERN_C int CALC_ICalculator2_sum(const struct CALC_ICalculator2* self, struct CALC_IStatus* status, int n1, int n2)
{
	return self->vtable->sum(self, status, n1, n2);
}

CLOOP_EXTERN_C int CALC_ICalculator2_getMemory(const struct CALC_ICalculator2* self)
{
	return self->vtable->getMemory(self);
}

CLOOP_EXTERN_C void CALC_ICalculator2_setMemory(struct CALC_ICalculator2* self, int n)
{
	self->vtable->setMemory(self, n);
}

CLOOP_EXTERN_C void CALC_ICalculator2_sumAndStore(struct CALC_ICalculator2* self, struct CALC_IStatus* status, int n1, int n2)
{
	self->vtable->sumAndStore(self, status, n1, n2);
}

CLOOP_EXTERN_C int CALC_ICalculator2_multiply(const struct CALC_ICalculator2* self, struct CALC_IStatus* status, int n1, int n2)
{
	return self->vtable->multiply(self, status, n1, n2);
}

CLOOP_EXTERN_C void CALC_ICalculator2_copyMemory(struct CALC_ICalculator2* self, const struct CALC_ICalculator* calculator)
{
	self->vtable->copyMemory(self, calculator);
}

CLOOP_EXTERN_C void CALC_ICalculator2_copyMemory2(struct CALC_ICalculator2* self, const int* address)
{
	self->vtable->copyMemory2(self, address);
}

