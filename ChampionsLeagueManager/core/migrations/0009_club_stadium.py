# Generated by Django 2.1.4 on 2019-01-11 14:24

from django.db import migrations, models


class Migration(migrations.Migration):

    dependencies = [
        ('core', '0008_redcard_double_yellow'),
    ]

    operations = [
        migrations.AddField(
            model_name='club',
            name='stadium',
            field=models.CharField(default='None', max_length=40),
        ),
    ]
