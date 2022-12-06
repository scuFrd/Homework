select categoryname, count(categoryname), round(avg(unitprice),2), min(unitprice), max(unitprice), sum(unitsonorder)
from(
select productname, categoryname, unitprice, unitsonorder
from product, category
where category.id = product.categoryid
group by categoryname, productname
order by categoryid
)
group by categoryname
having count(categoryname) > 10
;

