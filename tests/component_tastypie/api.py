from django.core.exceptions import ObjectDoesNotExist
from tastypie.resources import Resource
from tastypie.exceptions import NotFound


class SimpleResource(Resource):
    class Meta:
        resource_name = 'simple'

    def obj_get(self, *args, **kwargs):
        pk = kwargs['pk']
        if pk == 'NotFound':
            raise NotFound('Whatever it was you were looking for, it not here')
        elif pk == 'ObjectDoesNotExist':
            raise ObjectDoesNotExist('It really does not exist')
        elif pk == 'ZeroDivisionError':
            1 / 0
        else:
            raise NotImplemented()